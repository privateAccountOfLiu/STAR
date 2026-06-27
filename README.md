# STAR for Windows x86

[![STAR](https://img.shields.io/badge/STAR-alexdobin-39c5bb.svg)](https://github.com/alexdobin/STAR)

STAR (Spliced Transcripts Alignment to a Reference) 的 Windows 移植版本，基于原版 [alexdobin/STAR](https://github.com/alexdobin/STAR) v2.7.11b。

## 编译环境

- **编译器**：MinGW-w64 (x86_64-w64-mingw32-g++ 8.1+)
- **环境**：MSYS2 或 独立 MinGW-w64 + bash
- **依赖**：zlib (MinGW 自带), pthreads (winpthreads), OpenMP (libgomp)
- **构建工具**：GNU Make 4.4+, Python 3.x (替代 xxd)

## 编译

```bash
cd source
make STARwin
```

生成 `STAR.exe`（静态链接，无需外部 DLL）。

## 验证

以下测试在 Windows (MinGW-w64 15.2.0) 与 WSL2/Linux 之间逐字节一致：

```bash
# 下载测试基因组
curl -O ftp://ftp.ncbi.nlm.nih.gov/genomes/.../ecoli.fna.gz

# 建索引
STAR --runMode genomeGenerate --genomeDir ecoli_index --genomeFastaFiles ecoli.fna

# 比对
STAR --runMode alignReads --genomeDir ecoli_index --readFilesIn reads.fa

# 输出与 Linux 版 STAR 完全一致
```

## 从原版变更清单

### 新建文件 (3)

| 文件 | 说明 |
|------|------|
| `source/windows_compat.h` | Windows 兼容层头文件，提供 POSIX → Win32 类型/函数映射 |
| `scripts/xxd_i.py` | `xxd -i` 的 Python 替代方案，用于生成 `parametersDefault.xxd` |
| `README_Windows.md` | 本文件 |

### 修改的 STAR 核心文件 (11)

#### `source/IncludeDefine.h`
```diff
+ #ifndef _WIN32
  #include <sys/types.h>
  #include <sys/ipc.h>
  #include <sys/shm.h>
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <fcntl.h>
+ #else
+ #include "windows_compat.h"
+ #include <sys/stat.h>
+ #endif
```
**原因**：`sys/ipc.h`、`sys/shm.h`、`sys/mman.h` 在 MinGW 中不存在，替换为 Windows 兼容层。

#### `source/Parameters.h`
```diff
+ #ifndef _WIN32
  #include <unistd.h>
  #include <signal.h>
+ #endif

- pid_t readFilesCommandPID[MAX_N_MATES];
+ #ifdef _WIN32
+ HANDLE readFilesCommandPID[MAX_N_MATES];
+ #else
+ pid_t readFilesCommandPID[MAX_N_MATES];
+ #endif
```
**原因**：Windows 进程管理使用 `HANDLE` 而非 POSIX `pid_t`。

#### `source/SharedMemory.h`
```diff
  #include <string>
+ #ifndef _WIN32
  #include <semaphore.h>
  #include <unistd.h>
+ #else
+ #include "windows_compat.h"
+ #endif

+ #ifdef _WIN32
+     void *_hMapping;       // HANDLE
+     void *_hCounterMapping;
+ #else
      int _shmID;
      int _sharedCounterID;
+ #endif
-     sem_t *_sem;
+ #ifndef _WIN32
+     sem_t *_sem;
+ #endif
```
**原因**：Windows 使用 Named File Mapping 替代 SysV/POSIX 共享内存。

#### `source/SharedMemory.cpp`
重写为 `#ifdef _WIN32` / `#else` 双分支，共 ~300 行改动。

| Unix 原版 | Windows 替换 |
|-----------|-------------|
| `shmget()` / `shm_open()` | `CreateFileMappingW()` |
| `shmat()` / `mmap()` | `MapViewOfFile()` |
| `shmdt()` / `munmap()` | `UnmapViewOfFile()` |
| `shmctl(IPC_RMID)` / `shm_unlink()` | `CloseHandle()` (自动释放) |
| `shmctl(IPC_STAT)` (引用计数) | `InterlockedCompareExchange()` |
| `ftruncate()` | 无需 (`CreateFileMapping` 直接指定大小) |

Key 生成：Unix 使用 `st_ino` (inode 号)，Windows 改用 `std::hash<std::string>{}(path)`。

#### `source/Parameters_openReadsFiles.cpp`
```diff
+ #ifdef _WIN32
+ // 用 .bat 批处理 + 临时文件替代 FIFO + vfork/exec
+ CreateProcessA("cmd.exe /c batch.bat", ...)
+ WaitForSingleObject(...)
+ // 读取临时输出文件
+ #else
  // 原版 mkfifo + vfork + execlp 保留
+ #endif
```
**原因**：Windows 不支持 mkfifo/FIFO 和 vfork。改为：创建 .bat 脚本 → `CreateProcess` 同步执行 → 读取输出的临时文件。

#### `source/Parameters_closeReadsFiles.cpp`
```diff
- kill(readFilesCommandPID[imate], SIGKILL);
+ #ifdef _WIN32
+ TerminateProcess((HANDLE)readFilesCommandPID[imate], 1);
+ CloseHandle((HANDLE)readFilesCommandPID[imate]);
+ #else
+ kill(readFilesCommandPID[imate], SIGKILL);
+ #endif
```
**原因**：Windows 用 `TerminateProcess` 终止子进程，之后需 `CloseHandle` 释放。

#### `source/Parameters_readSAMheader.cpp`
```diff
+ #ifdef _WIN32
+ // 用临时文件替代 FIFO
+ system("command > tmpfile")
+ ifstream tmpIn(tmpfile)
+ #else
  // 原版 mkfifo + system(&) 异步管道保留
+ #endif
```
**原因**：Windows 无 mkfifo。改用阻塞式 `system()` + 临时文件。

#### `source/Genome.cpp`
```diff
- shmKey = stbuf.st_ino;
+ #ifdef _WIN32
+ shmKey = std::hash<std::string>{}(pGe.gDir);
+ #else
+ shmKey = stbuf.st_ino;
+ #endif
```
**原因**：Windows 文件系统无 inode 概念，改用目录路径 hash 生成共享内存 key。

#### `source/sysRemoveDir.cpp`
```diff
+ #ifdef _WIN32
+ // 递归 FindFirstFile/FindNextFile 实现
+ void sysRemoveDir(std::string dirName) {
+     WIN32_FIND_DATAA fd;
+     HANDLE hFind = FindFirstFileA(...);
+     // 递归删除子目录和文件
+     RemoveDirectoryA(...);
+ }
+ #else
  // 原版 nftw() 实现保留
+ #endif
```
**原因**：`nftw()` 是 POSIX 特有函数，Windows 需用 `FindFirstFile`/`FindNextFile` 递归遍历。

#### `source/systemFunctions.cpp`
```diff
+ #ifdef _WIN32
+ std::string windowsProcMemory() {
+     PROCESS_MEMORY_COUNTERS_EX pmc;
+     GetProcessMemoryInfo(GetCurrentProcess(), ...);
+     // 格式化为 VmPeak/VmSize/VmHWM/VmRSS
+ }
+ std::string linuxProcMemory() { return windowsProcMemory(); }
+ #else
  // 原版读取 /proc/self/status 保留
+ #endif
```
**原因**：`/proc/self/status` 是 Linux 特有。Windows 用 `GetProcessMemoryInfo()`。

#### `source/SoloFeature_outputResults.cpp`
```diff
- symlink(sjout.c_str(), target.c_str())
+ #ifdef _WIN32
+ CopyFileA(sjout.c_str(), target.c_str(), FALSE)  // 无符号链接权限，改用复制
+ #else
+ symlink(sjout.c_str(), target.c_str())
+ #endif
```

#### `source/systemFunctions.h`
```diff
+ #ifdef _WIN32
+ std::string windowsProcMemory();
+ #endif
```

#### `source/streamFuns.cpp`
```diff
- std::ofstream(fileName.c_str(), std::fstream::out | std::fstream::trunc)
+ std::ofstream(fileName.c_str(), std::fstream::out | std::fstream::trunc | std::ios::binary)

- std::fstream(fileName.c_str(), std::fstream::in | std::fstream::out | std::fstream::trunc)
+ std::fstream(..., ... | std::ios::binary)

- ifstream saChunkFile(path);   // Genome_genomeGenerate.cpp:312 读取 SA 分块
+ ifstream saChunkFile(path, std::ios::binary);
```
**原因**：Windows 默认以文本模式打开文件，会将 `0x1A` (Ctrl-Z) 视为 EOF 并将 `\n`
转换为 `\r\n`，导致二进制基因组索引数据损坏。这是最关键的修复。

#### `source/Genome_genomeGenerate.cpp`
- SA 分块读取改用 `std::ios::binary` 模式

### 修改的 htslib 文件 (5)

#### `source/htslib/config.h`
```diff
+ #define HAVE_FSEEKO
+ #ifdef _WIN32
+ #include <stdlib.h>
+ #include <io.h>
+ #define drand48() ((double)rand()/(double)RAND_MAX)
+ #define fsync(fd) _commit(fd)
+ #define mkdir(path, mode) mkdir(path)
+ #endif
```
**原因**：`drand48` 是 POSIX 函数；`fsync` 在 Windows 上需用 `_commit`；`mkdir` 需单参数版本；`HAVE_FSEEKO` 规避 MinGW 的 `_fseeki64` 类型冲突。

#### `source/htslib/hfile.c`
```diff
+ #ifndef _WIN32
  #include <sys/socket.h>
+ #endif

  #ifdef _WIN32
+ #include <winsock2.h>
  #define HAVE_CLOSESOCKET
+ #define fsync(fd) _commit(fd)
  #endif

  // blksize()
+ #ifdef _WIN32
+ return 4096;  // NTFS 默认块大小
+ #else
  return sbuf.st_blksize;
+ #endif
```
**原因**：MinGW 无 `sys/socket.h`，winsock2 需要显式包含；`st_blksize` 在 Windows struct stat 中不存在。

#### `source/htslib/hts.c`
```diff
+ #include "config.h"
```
**原因**：使 `config.h` 中的 `drand48` 宏在 hts.c 编译时生效。

#### `source/htslib/cram/cram_io.c`
```diff
+ #include "config.h"
```
**原因**：使 `mkdir` 和 `fsync` 宏生效。

#### `source/htslib/cram/os.h`
```diff
+ #include "../config.h"
```
**原因**：使所有 CRAM 源文件继承 config.h 中的 Windows 兼容宏。

### 修改的构建文件 (1)

#### `source/Makefile`
```diff
+ # Windows 构建时间/路径
+ ifdef OS
+     BUILD_DATE ?= $(shell powershell ...)
+     BUILD_PLACE ?= $(COMPUTERNAME):$(CURDIR)
+ endif

+ # xxd 替代 (xxd_i.py)
+ parametersDefault.xxd:
+     @which xxd ... || python .../xxd_i.py ...

+ # Windows 构建目标
+ STARwin :
+     LDFLAGS := -static -pthread -Lhtslib -lhts -lz -lws2_32
+     $(CXX) -o STAR.exe ...
+
+ STARlongWin : ...
```

### 修改汇总

| 类别 | 新建 | 修改 | 关键问题 |
|------|------|------|---------|
| 兼容层 | 1 个 (.h) | - | POSIX → Win32 类型/宏映射 |
| 共享内存 | - | 2 个 (.h, .cpp) | SysV/POSIX → Named File Mapping |
| 进程管理 | - | 3 个 (.cpp) | vfork/exec → CreateProcess, kill → TerminateProcess |
| 文件系统 | - | 4 个 (.cpp, .h) | nftw, statvfs, symlink, /proc |
| 文件 I/O | - | 2 个 (.cpp) | ios::binary 防止数据损坏 |
| htslib | - | 5 个 (.c, .h) | 套接字, drand48, fsync, mkdir, fseeko |
| 构建 | 1 个 (.py) | 1 个 (Makefile) | xxd 替代, Windows 链接标志 |
| **合计** | **3** | **17** | |

## 已知限制

1. **共享内存**：Windows 命名文件映射 (`CreateFileMapping`) 工作正常，但与 Linux/SysV 共享内存不兼容（跨平台进程间共享基因组索引需要 `--genomeLoad NoSharedMemory`）
2. **符号链接**：Windows 普通用户无创建符号链接权限，`symlink()` 改为文件复制
3. **管道输入**：`--readFilesCommand` 预处理命令通过临时文件执行，而非 FIFO
4. **仅 MinGW**：不支持 MSVC（需 `__uint128_t` GCC 扩展）

## License

与原版 STAR 保持一致 (MIT)。参见 [LICENSE](LICENSE)。

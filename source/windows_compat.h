#ifndef WINDOWS_COMPAT_DEF
#define WINDOWS_COMPAT_DEF

// MinGW/Windows compatibility header for STAR
// MinGW already provides most POSIX headers and functions.
// This header only fills the remaining gaps.

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#include <sys/stat.h>
#include <pthread.h>

// key_t: POSIX IPC type, not in MinGW's sys/types.h
// We use unsigned long long to hold hash values
typedef unsigned long long key_t;

// MinGW io.h may or may not define mkdir as 1-arg macro.
// Ensure consistency: map 2-arg POSIX mkdir to 1-arg _mkdir
#ifndef mkdir
#define mkdir(path, mode) _mkdir(path)
#endif

// st_blksize not available in MinGW stat
// (used only in htslib/hfile.c, handled there)

// struct statvfs — not in MinGW; stub for streamFuns.cpp
// The compat implementation calls GetDiskFreeSpaceExA
struct statvfs {
    unsigned long long f_bavail;
    unsigned long long f_bsize;
};

static inline int statvfs(const char *path, struct statvfs *buf) {
    ULARGE_INTEGER freeBytes, total, totalFree;
    if (!GetDiskFreeSpaceExA(path, &freeBytes, &total, &totalFree))
        return -1;
    buf->f_bavail = freeBytes.QuadPart;
    buf->f_bsize = 1;
    return 0;
}

// getopt_long — not in MinGW; declare stub prototype
// (only used in htslib bgzip.c/tabix.c which are not linked into STAR)
#ifdef __cplusplus
extern "C" {
#endif
struct option {
    const char *name;
    int has_arg;
    int *flag;
    int val;
};
extern int getopt_long(int argc, char * const *argv,
                       const char *optstring,
                       const struct option *longopts, int *longindex);
#ifdef __cplusplus
}
#endif

#endif // _WIN32

#endif // WINDOWS_COMPAT_DEF

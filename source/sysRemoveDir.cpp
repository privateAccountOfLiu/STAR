#include <string>
#include <cstring>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void sysRemoveDir(std::string dirName) {
    // Recursive directory deletion using FindFirstFile/FindNextFile
    std::string searchPath = dirName + "\\*";

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        RemoveDirectoryA(dirName.c_str());
        return;
    }

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;

        std::string fullPath = dirName + "\\" + fd.cFileName;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            sysRemoveDir(fullPath);
        } else {
            SetFileAttributesA(fullPath.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileA(fullPath.c_str());
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
    SetFileAttributesA(dirName.c_str(), FILE_ATTRIBUTE_NORMAL);
    RemoveDirectoryA(dirName.c_str());
}
#else
// Unix: original nftw-based implementation
#include <ftw.h>
#include <unistd.h>

int removeFileOrDir(const char *fpath,const struct stat *sb, int typeflag, struct FTW *ftwbuf) {

    {//to avoid unused variable warning
        (void) sb;
        (void) ftwbuf;
    };

    if (typeflag==FTW_F) {//file
        remove(fpath);
    } else if (typeflag==FTW_DP) {//dir
        rmdir(fpath);
    } else {//something went wrong, stop the removal
        return -1;
    };
    return 0;
};


void sysRemoveDir(std::string dirName) {//remove directory and all its contents
    int nftwFlag=FTW_DEPTH;
    nftw(dirName.c_str(), removeFileOrDir, 100, nftwFlag);
};
#endif

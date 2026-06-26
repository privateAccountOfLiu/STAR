#include "Parameters.h"
#include "ErrorWarning.h"
#include <fstream>
#include <sys/stat.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

void Parameters::closeReadsFiles() {
    for (uint imate=0; imate<readFilesIn.size(); imate++) {//open readIn files
        if ( inOut->readIn[imate].is_open() )
            inOut->readIn[imate].close();
        if (readFilesCommandPID[imate] != NULL) {
#ifdef _WIN32
            TerminateProcess((HANDLE)readFilesCommandPID[imate], 1);
            CloseHandle((HANDLE)readFilesCommandPID[imate]);
#else
            kill(readFilesCommandPID[imate],SIGKILL);
#endif
        }
    };
};
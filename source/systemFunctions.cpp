// system functions
#include <string>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#endif

#ifdef _WIN32
std::string windowsProcMemory()
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    std::string outString;

    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        std::ostringstream buf;
        buf << "VmPeak: " << (pmc.PeakWorkingSetSize / 1024) << " kB; ";
        buf << "VmSize: " << (pmc.WorkingSetSize / 1024) << " kB; ";
        buf << "VmHWM: " << (pmc.PeakWorkingSetSize / 1024) << " kB; ";
        buf << "VmRSS: " << (pmc.WorkingSetSize / 1024) << " kB; ";
        outString = buf.str();
    }
    outString += '\n';
    return outString;
}

// Provide stub for linuxProcMemory on Windows
std::string linuxProcMemory()
{
    return windowsProcMemory();
}
#else
std::string linuxProcMemory()
{
    std::ifstream t("/proc/self/status");
    std::stringstream buffer;
    buffer << t.rdbuf();

    std::string outString;
    while (buffer.good()) {
        std::string str1;
        std::getline(buffer,str1);
        if ( (str1.rfind("VmPeak",0) == 0) ||
             (str1.rfind("VmSize",0) == 0) ||
             (str1.rfind("VmHWM",0) == 0)  ||
             (str1.rfind("VmRSS",0) == 0) ) {
                 outString += str1+"; ";
             };
    };
    outString += '\n';

    return outString;
};
#endif

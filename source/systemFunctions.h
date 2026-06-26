#ifndef CODE_systemFunctions
#include <string>

std::string linuxProcMemory();
#ifdef _WIN32
std::string windowsProcMemory();
#endif

#endif

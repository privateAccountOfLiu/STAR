#define _USE_KNETFILE
#define BGZF_CACHE
#define BGZF_MT
#define HAVE_FSEEKO

#ifdef _WIN32
#include <stdlib.h>
#include <io.h>
#define drand48() ((double)rand() / (double)RAND_MAX)
#define fsync(fd) _commit(fd)
#define mkdir(path, mode) mkdir(path)
#endif

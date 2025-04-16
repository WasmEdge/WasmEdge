#include "common/defines.h"
#if WASMEDGE_OS_FREEBSD
#include "inode-freebsd.cpp"
#elif WASMEDGE_OS_LINUX
#include "inode-linux.cpp"
#else
#error
#endif

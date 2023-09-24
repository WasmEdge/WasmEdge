#include <common/defines.h>

#if WASMEDGE_OS_LINUX
#include "wasi_random/module.h"

static ssize_t getrandom_wrapper(void *Buf, size_t Len, unsigned int Flags);

#if defined __GLIBC__ && defined __linux__
#if __GLIBC__ > 2 || __GLIBC_MINOR__ > 24
#include <sys/random.h>
static ssize_t getrandom_wrapper(void *Buf, size_t Len, unsigned int Flags) {
  return getrandom(Buf, Len, Flags);
}
#else
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
static ssize_t getrandom_wrapper(void *Buf, size_t Len, unsigned int Flags) {
  return syscall(SYS_getrandom, Buf, Len, Flags);
}
#endif
#else /*not glibc*/
static ssize_t getrandom_wrapper(void *, size_t, unsigned int) { return -1; }
#endif

#ifndef GRND_NONBLOCK
#define GRND_NONBLOCK 0x01
#endif

#ifndef GRND_RANDOM
#define GRND_RANDOM 0x02
#endif

#ifndef GRND_INSECURE
#define GRND_INSECURE 0x04
#endif

namespace WasmEdge {
namespace Host {

bool WasiRandomEnvironment::getRandomBytes(uint64_t Len, uint8_t *Buf) {
  ssize_t RealQuerySize =
      getrandom_wrapper(Buf, Len, GRND_RANDOM | GRND_NONBLOCK);

  // note: the spec did not handle the error such like EAGAIN, we just make that
  // case fail.
  if (RealQuerySize < 0 || RealQuerySize != static_cast<ssize_t>(Len))
    return false;

  return true;
}

bool WasiRandomEnvironment::getInsecureRandomBytes(uint64_t Len, uint8_t *Buf) {
  ssize_t RealQuerySize =
      getrandom_wrapper(Buf, Len, GRND_INSECURE | GRND_NONBLOCK);

  // note: the spec did not handle the error such like EAGAIN, we just make that
  // case fail.
  if (RealQuerySize < 0 || RealQuerySize != static_cast<ssize_t>(Len))
    return false;

  return true;
}

} // namespace Host
} // namespace WasmEdge
#endif

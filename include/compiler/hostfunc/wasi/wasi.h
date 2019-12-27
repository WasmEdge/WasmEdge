// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "compiler/hostfunc.h"
#include "vm/environment.h"
#include "wasi/core.h"

namespace SSVM {
namespace Compiler {

class Wasi : public HostFunction {
public:
  Wasi(Library &Lib, VM::WasiEnvironment &HostEnv)
      : HostFunction(Lib), Env(HostEnv) {}

protected:
  VM::WasiEnvironment &Env;

  static constexpr uint32_t convertErrno(uint32_t Error) {
    switch (Error) {
#define X(V)                                                                   \
  case V:                                                                      \
    return __WASI_##V;
      X(E2BIG)
      X(EACCES)
      X(EADDRINUSE)
      X(EADDRNOTAVAIL)
      X(EAFNOSUPPORT)
      X(EAGAIN)
      X(EALREADY)
      X(EBADF)
      X(EBADMSG)
      X(EBUSY)
      X(ECANCELED)
      X(ECHILD)
      X(ECONNABORTED)
      X(ECONNREFUSED)
      X(ECONNRESET)
      X(EDEADLK)
      X(EDESTADDRREQ)
      X(EDOM)
      X(EDQUOT)
      X(EEXIST)
      X(EFAULT)
      X(EFBIG)
      X(EHOSTUNREACH)
      X(EIDRM)
      X(EILSEQ)
      X(EINPROGRESS)
      X(EINTR)
      X(EINVAL)
      X(EIO)
      X(EISCONN)
      X(EISDIR)
      X(ELOOP)
      X(EMFILE)
      X(EMLINK)
      X(EMSGSIZE)
      X(EMULTIHOP)
      X(ENAMETOOLONG)
      X(ENETDOWN)
      X(ENETRESET)
      X(ENETUNREACH)
      X(ENFILE)
      X(ENOBUFS)
      X(ENODEV)
      X(ENOENT)
      X(ENOEXEC)
      X(ENOLCK)
      X(ENOLINK)
      X(ENOMEM)
      X(ENOMSG)
      X(ENOPROTOOPT)
      X(ENOSPC)
      X(ENOSYS)
#ifdef ENOTCAPABLE
      X(ENOTCAPABLE)
#endif
      X(ENOTCONN)
      X(ENOTDIR)
      X(ENOTEMPTY)
      X(ENOTRECOVERABLE)
      X(ENOTSOCK)
      X(ENOTSUP)
      X(ENOTTY)
      X(ENXIO)
      X(EOVERFLOW)
      X(EOWNERDEAD)
      X(EPERM)
      X(EPIPE)
      X(EPROTO)
      X(EPROTONOSUPPORT)
      X(EPROTOTYPE)
      X(ERANGE)
      X(EROFS)
      X(ESPIPE)
      X(ESRCH)
      X(ESTALE)
      X(ETIMEDOUT)
      X(ETXTBSY)
      X(EXDEV)
#undef X
#if EOPNOTSUPP != ENOTSUP
    case EOPNOTSUPP:
      return __WASI_ENOTSUP;
#endif
#if EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
      return __WASI_EAGAIN;
#endif
    default:
      return __WASI_ENOSYS;
    };
  }
};

} // namespace Compiler
} // namespace SSVM

// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "processfunc.h"

#include "common/defines.h"

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#elif WASMEDGE_OS_WINDOWS
#endif

namespace WasmEdge {
namespace Host {

Expect<void>
WasmEdgeProcessSetProgName::body(const Runtime::CallingFrame &Frame,
                                 uint32_t NamePtr, uint32_t NameLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto Buf = MemInst->getSpan<const char>(NamePtr, NameLen);
  std::copy(Buf.begin(), Buf.end(), std::back_inserter(Env.Name));
  return {};
}

Expect<void> WasmEdgeProcessAddArg::body(const Runtime::CallingFrame &Frame,
                                         uint32_t ArgPtr, uint32_t ArgLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto Buf = MemInst->getSpan<const char>(ArgPtr, ArgLen);
  std::string NewArg;
  std::copy(Buf.begin(), Buf.end(), std::back_inserter(NewArg));
  Env.Args.push_back(std::move(NewArg));
  return {};
}

Expect<void> WasmEdgeProcessAddEnv::body(const Runtime::CallingFrame &Frame,
                                         uint32_t EnvNamePtr,
                                         uint32_t EnvNameLen,
                                         uint32_t EnvValPtr,
                                         uint32_t EnvValLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto EnvBuf = MemInst->getSpan<const char>(EnvNamePtr, EnvNameLen);
  const auto ValBuf = MemInst->getSpan<const char>(EnvValPtr, EnvValLen);
  std::string NewEnv, NewVal;
  std::copy(EnvBuf.begin(), EnvBuf.end(), std::back_inserter(NewEnv));
  std::copy(ValBuf.begin(), ValBuf.end(), std::back_inserter(NewVal));
  Env.Envs.emplace(std::move(NewEnv), std::move(NewVal));
  return {};
}

Expect<void> WasmEdgeProcessAddStdIn::body(const Runtime::CallingFrame &Frame,
                                           uint32_t BufPtr, uint32_t BufLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto const Buf = MemInst->getSpan<uint8_t>(BufPtr, BufLen);
  Env.StdIn.reserve(Env.StdIn.size() + BufLen);
  std::copy(Buf.begin(), Buf.end(), std::back_inserter(Env.StdIn));
  return {};
}

Expect<void> WasmEdgeProcessSetTimeOut::body(const Runtime::CallingFrame &,
                                             uint32_t Time) {
  Env.TimeOut = Time;
  return {};
}

Expect<uint32_t> WasmEdgeProcessRun::body(const Runtime::CallingFrame &) {
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
  // Clear outputs.
  Env.StdOut.clear();
  Env.StdErr.clear();
  Env.ExitCode = static_cast<uint32_t>(-1);

  // Check white list of commands.
  if (!Env.AllowedAll &&
      Env.AllowedCmd.find(Env.Name) == Env.AllowedCmd.end()) {
    std::string Msg = "Permission denied: Command \"";
    Msg.append(Env.Name);
    Msg.append("\" is not in the white list. Please use --allow-command=");
    Msg.append(Env.Name);
    Msg.append(" or --allow-command-all to add \"");
    Msg.append(Env.Name);
    Msg.append("\" command into the white list.\n");
    Env.Name.clear();
    Env.Args.clear();
    Env.Envs.clear();
    Env.StdIn.clear();
    Env.StdErr.reserve(Msg.length());
    std::copy_n(Msg.c_str(), Msg.length(), std::back_inserter(Env.StdErr));
    Env.ExitCode = static_cast<int32_t>(INT8_C(-1));
    Env.TimeOut = Env.DEFAULT_TIMEOUT;
    return Env.ExitCode;
  }

  // Create pipes for stdin, stdout, and stderr.
  int FDStdIn[2], FDStdOut[2], FDStdErr[2];
  if (pipe(FDStdIn) == -1) {
    // Create stdin pipe failed.
    return Env.ExitCode;
  }
  if (pipe(FDStdOut) == -1) {
    // Create stdout pipe failed.
    close(FDStdIn[0]);
    close(FDStdIn[1]);
    return Env.ExitCode;
  }
  if (pipe(FDStdErr) == -1) {
    // Create stderr pipe failed.
    close(FDStdIn[0]);
    close(FDStdIn[1]);
    close(FDStdOut[0]);
    close(FDStdOut[1]);
    return Env.ExitCode;
  }

  // Create a child process for executing command.
  pid_t PID = fork();
  if (PID == -1) {
    // Create process failed.
    close(FDStdIn[0]);
    close(FDStdIn[1]);
    close(FDStdOut[0]);
    close(FDStdOut[1]);
    close(FDStdErr[0]);
    close(FDStdErr[1]);
    return Env.ExitCode;
  } else if (PID == 0) {
    // Child process. Setup pipes.
    dup2(FDStdIn[0], 0);
    dup2(FDStdOut[1], 1);
    dup2(FDStdErr[1], 2);
    close(FDStdIn[0]);
    close(FDStdIn[1]);
    close(FDStdOut[0]);
    close(FDStdOut[1]);
    close(FDStdErr[0]);
    close(FDStdErr[1]);

    // Prepare arguments and environment variables.
    std::vector<std::string> EnvStr;
    for (auto &It : Env.Envs) {
      EnvStr.push_back(It.first + "=" + It.second);
    }
    std::vector<char *> Argv, Envp;
    Argv.push_back(Env.Name.data());
    std::transform(Env.Args.begin(), Env.Args.end(), std::back_inserter(Argv),
                   [](std::string &S) { return S.data(); });
    std::transform(EnvStr.begin(), EnvStr.end(), std::back_inserter(Envp),
                   [](std::string &S) { return S.data(); });
    Argv.push_back(nullptr);
    Envp.push_back(nullptr);
#if defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 11)
    if (execvpe(Env.Name.c_str(), &Argv[0], &Envp[0]) == -1) {
#else
    if (execve(Env.Name.c_str(), &Argv[0], &Envp[0]) == -1) {
#endif
#else
    if (execve(Env.Name.c_str(), &Argv[0], &Envp[0]) == -1) {
#endif
      switch (errno) {
      case EACCES:
        spdlog::error("Permission denied.");
        break;
      case ENOENT:
        spdlog::error("Command not found.");
        break;
      default:
        spdlog::error("Unknown error.");
        break;
      }
      _exit(-1);
    }
  } else {
    // Parent process. Close unused file descriptors.
    close(FDStdIn[0]);
    close(FDStdOut[1]);
    close(FDStdErr[1]);

    // Send inputs.
    uint32_t WBytes = 0;
    while (WBytes < Env.StdIn.size()) {
      uint32_t WriteNum =
          std::min(static_cast<size_t>(PIPE_BUF), Env.StdIn.size() - WBytes);
      if (auto Res = write(FDStdIn[1], &Env.StdIn[WBytes], WriteNum); Res > 0) {
        WBytes += Res;
      } else {
        break;
      }
    }
    close(FDStdIn[1]);

    // Waiting for child process and get outputs.
    uint8_t Buf[PIPE_BUF];
    ssize_t RBytes;
    int ChildStat;
    struct timeval TStart, TCurr;
    gettimeofday(&TStart, NULL);
    while (true) {
      gettimeofday(&TCurr, NULL);
      if ((TCurr.tv_sec - TStart.tv_sec) * 1000U +
              (TCurr.tv_usec - TStart.tv_usec) / 1000000U >
          Env.TimeOut) {
        // Over timeout. Interrupt child process.
        kill(PID, SIGKILL);
        Env.ExitCode = static_cast<uint32_t>(ETIMEDOUT);
        break;
      }

      // Wait for child process.
      pid_t WPID = waitpid(PID, &ChildStat, WNOHANG);
      if (WPID == -1) {
        // waitpid failed.
        Env.ExitCode = static_cast<uint32_t>(EINVAL);
        break;
      } else if (WPID > 0) {
        // Child process returned.
        Env.ExitCode = static_cast<int8_t>(WEXITSTATUS(ChildStat));
        break;
      }

      // Read stdout and stderr.
      fd_set FDSet;
      int NFD = std::max(FDStdOut[0], FDStdErr[0]) + 1;
      FD_ZERO(&FDSet);
      FD_SET(FDStdOut[0], &FDSet);
      FD_SET(FDStdErr[0], &FDSet);
      struct timeval TSelect = {.tv_sec = 0, .tv_usec = 0};
      if (select(NFD, &FDSet, NULL, NULL, &TSelect) > 0) {
        if (FD_ISSET(FDStdOut[0], &FDSet)) {
          if (RBytes = read(FDStdOut[0], Buf, sizeof(Buf)); RBytes > 0) {
            Env.StdOut.reserve(Env.StdOut.size() + RBytes);
            std::copy_n(Buf, RBytes, std::back_inserter(Env.StdOut));
          }
        }
        if (FD_ISSET(FDStdErr[0], &FDSet)) {
          if (RBytes = read(FDStdErr[0], Buf, sizeof(Buf)); RBytes > 0) {
            Env.StdErr.reserve(Env.StdErr.size() + RBytes);
            std::copy_n(Buf, RBytes, std::back_inserter(Env.StdErr));
          }
        }
      }
      usleep(Env.DEFAULT_POLLTIME * 1000);
    }

    // Read remained stdout and stderr.
    do {
      RBytes = read(FDStdOut[0], Buf, sizeof(Buf));
      if (RBytes > 0) {
        Env.StdOut.reserve(Env.StdOut.size() + RBytes);
        std::copy_n(Buf, RBytes, std::back_inserter(Env.StdOut));
      }
    } while (RBytes > 0);
    do {
      RBytes = read(FDStdErr[0], Buf, sizeof(Buf));
      if (RBytes > 0) {
        Env.StdErr.reserve(Env.StdErr.size() + RBytes);
        std::copy_n(Buf, RBytes, std::back_inserter(Env.StdErr));
      }
    } while (RBytes > 0);
    close(FDStdOut[0]);
    close(FDStdErr[0]);
  }

  // Reset inputs.
  Env.Name.clear();
  Env.Args.clear();
  Env.Envs.clear();
  Env.StdIn.clear();
  Env.TimeOut = Env.DEFAULT_TIMEOUT;
  return Env.ExitCode;
#elif WASMEDGE_OS_WINDOWS
  spdlog::error("wasmedge_process doesn't support windows now.");
  return Unexpect(ErrCode::Value::HostFuncError);
#endif
}

Expect<uint32_t>
WasmEdgeProcessGetExitCode::body(const Runtime::CallingFrame &) {
  return Env.ExitCode;
}

Expect<uint32_t>
WasmEdgeProcessGetStdOutLen::body(const Runtime::CallingFrame &) {
  return static_cast<uint32_t>(Env.StdOut.size());
}

Expect<void> WasmEdgeProcessGetStdOut::body(const Runtime::CallingFrame &Frame,
                                            uint32_t BufPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto Buf = MemInst->getSpan<char>(BufPtr, Env.StdOut.size());
  std::copy_n(Env.StdOut.begin(), std::min(Env.StdOut.size(), Buf.size()),
              Buf.begin());
  return {};
}

Expect<uint32_t>
WasmEdgeProcessGetStdErrLen::body(const Runtime::CallingFrame &) {
  return static_cast<uint32_t>(Env.StdErr.size());
}

Expect<void> WasmEdgeProcessGetStdErr::body(const Runtime::CallingFrame &Frame,
                                            uint32_t BufPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const auto Buf = MemInst->getSpan<char>(BufPtr, Env.StdErr.size());
  std::copy_n(Env.StdErr.begin(), std::min(Env.StdErr.size(), Buf.size()),
              Buf.begin());
  return {};
}

} // namespace Host
} // namespace WasmEdge

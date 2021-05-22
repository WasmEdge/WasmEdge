// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi/vfs-linux.h"
#include "host/wasi/vfs-macos.h"
#include "host/wasi/vfs-windows.h"
#include "host/wasi/vfs.h"
#include "wasi/api.hpp"
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace Host {

class WasiEnvironment {
public:
  WasiEnvironment();
  virtual ~WasiEnvironment() noexcept;

  void init(Span<const std::string> Dirs, std::string ProgramName,
            Span<const std::string> Args, Span<const std::string> Envs);
  void fini() noexcept;

  const std::vector<std::string> &getCmdArgs() const { return CmdArgs; }
  const std::vector<std::string> &getEnvirons() const { return Environs; }
  int getExitCode() const { return ExitCode; }
  void setExitCode(int ExitCode) { this->ExitCode = ExitCode; }

  template <typename... Args> void emplaceFile(__wasi_fd_t Fd, Args &&...args) {
    FileMap.emplace(std::piecewise_construct, std::forward_as_tuple(Fd),
                    std::forward_as_tuple(std::forward<Args>(args)...));
  }
  using FileIterator = std::map<__wasi_fd_t, WasiFile>::iterator;
  FileIterator getFile(__wasi_fd_t Fd) noexcept { return FileMap.find(Fd); }
  FileIterator getFileEnd() noexcept { return FileMap.end(); }
  void eraseFile(FileIterator File) noexcept { FileMap.erase(File); }
  __wasi_fd_t getNewFd() noexcept { return FileMap.rbegin()->first + 1; }
  void changeFd(FileIterator File, uint32_t Fd) {
    auto Node = FileMap.extract(File);
    Node.key() = Fd;
    FileMap.insert(std::move(Node));
  }

private:
  std::vector<std::string> CmdArgs;
  std::vector<std::string> Environs;
  std::map<__wasi_fd_t, WasiFile> FileMap;
  int ExitCode = 0;
};

} // namespace Host
} // namespace WasmEdge

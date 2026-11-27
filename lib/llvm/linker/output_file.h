// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "common/span.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

struct TempFile {
  std::filesystem::path Path;
  int File = -1;
};

/// Create a new file named ".wasmedge-" plus 16 random hex digits and Suffix
/// in the directory of Output. Only the random part changes between attempts.
Expect<TempFile> createUniqueSibling(const std::filesystem::path &Output,
                                     std::string_view Suffix = {},
                                     unsigned Mode = 0666);

/// Own a temporary file: close its descriptor and remove it unless published.
class TempGuard {
public:
  TempGuard(std::filesystem::path Value, int File) noexcept;
  TempGuard(const TempGuard &) = delete;
  TempGuard &operator=(const TempGuard &) = delete;
  ~TempGuard() noexcept;

  void publish() noexcept { Published = true; }
  const std::filesystem::path &path() const noexcept { return Path; }
  int release() noexcept {
    const int Result = File;
    File = -1;
    return Result;
  }

private:
  std::filesystem::path Path;
  int File;
  bool Published = false;
};

/// Run Program with Arguments and wait for it. Fail unless it exits with 0.
Expect<void> runProcess(const std::string &Program,
                        Span<const std::string> Arguments) noexcept;

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge

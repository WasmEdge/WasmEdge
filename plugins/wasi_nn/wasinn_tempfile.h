// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/defines.h"
#include "common/filesystem.h"
#include "common/span.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

#if WASMEDGE_OS_WINDOWS
#include <fstream>
#include <random>
#else
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#endif

namespace WasmEdge {
namespace Host {
namespace WASINN {

// The backends which load a model from a path only write the model bytes of a
// guest into a file of their own first. The file gets a unique name in the
// temporary directory of the host, so the guest names no path, and it is
// removed with this object.
class TempModelFile {
public:
  TempModelFile() noexcept = default;
  TempModelFile(const TempModelFile &) = delete;
  TempModelFile &operator=(const TempModelFile &) = delete;
  ~TempModelFile() noexcept { remove(); }

  // Create the file, named with the suffix at its end, and write the data.
  bool create(Span<const uint8_t> Data, std::string_view Suffix) noexcept {
    remove();
    std::error_code EC;
    const auto Dir = std::filesystem::temp_directory_path(EC);
    if (EC) {
      return false;
    }
#if WASMEDGE_OS_WINDOWS
    // The temporary directory belongs to the user, and a name of 128 random
    // bits is not one another process can plant a file under.
    static constexpr std::string_view Hex = "0123456789abcdef";
    std::random_device Rand;
    std::string Name = "wasmedge-wasinn-";
    for (int I = 0; I < 32; ++I) {
      Name += Hex[Rand() & 0xFU];
    }
    Name += Suffix;
    const auto File = Dir / u8path(Name);
    Path = File.u8string();
    std::ofstream Out(File, std::ios::binary | std::ios::trunc);
    Out.write(reinterpret_cast<const char *>(Data.data()),
              static_cast<std::streamsize>(Data.size()));
    Out.close();
    if (!Out) {
      remove();
      return false;
    }
    return true;
#else
    // Created exclusively and readable by the owner only.
    std::string Name = (Dir / "wasmedge-wasinn-XXXXXX").string();
    Name += Suffix;
    const int Fd = ::mkstemps(Name.data(), static_cast<int>(Suffix.size()));
    if (Fd < 0) {
      return false;
    }
    Path = std::move(Name);
    constexpr std::size_t WriteMax = std::size_t{1} << 30;
    std::size_t Size = 0;
    while (Size < Data.size()) {
      const auto Part = ::write(Fd, Data.data() + Size,
                                std::min(Data.size() - Size, WriteMax));
      if (Part < 0 && errno == EINTR) {
        continue;
      }
      if (Part <= 0) {
        break;
      }
      Size += static_cast<std::size_t>(Part);
    }
    if (::close(Fd) != 0 || Size != Data.size()) {
      remove();
      return false;
    }
    return true;
#endif
  }

  // The path of the file, or empty without one.
  const std::string &path() const noexcept { return Path; }

  void remove() noexcept {
    if (!Path.empty()) {
      std::error_code EC;
      std::filesystem::remove(u8path(Path), EC);
      Path.clear();
    }
  }

private:
  std::string Path;
};

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge

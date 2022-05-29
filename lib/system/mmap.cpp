// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "system/mmap.h"

#include "common/config.h"
#include "common/defines.h"

#include <cstdint>
#include <memory>
#include <utility>

#ifdef HAVE_MMAP
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#elif WASMEDGE_OS_WINDOWS
#include <boost/winapi/access_rights.hpp>
#include <boost/winapi/file_management.hpp>
#include <boost/winapi/file_mapping.hpp>
#include <boost/winapi/handles.hpp>
#include <boost/winapi/page_protection_flags.hpp>
#endif

namespace WasmEdge {

namespace {
#ifdef HAVE_MMAP
static inline bool kSupported = true;
struct Implement {
  void *Address = MAP_FAILED;
  int File = -1;
  uint64_t Size = 0;
  Implement(const std::filesystem::path &Path) noexcept {
    File = open(Path.native().c_str(), O_RDONLY);
    if (File < 0) {
      return;
    }

    {
      struct stat Stat;
      if (fstat(File, &Stat) < 0) {
        return;
      }
      Size = Stat.st_size;
    }

    Address = mmap(nullptr, Size, PROT_READ, MAP_SHARED, File, 0);
  }
  ~Implement() noexcept {
    if (Address != MAP_FAILED) {
      munmap(Address, Size);
    }
    if (File >= 0) {
      close(File);
    }
  }
  bool ok() const noexcept { return Address != MAP_FAILED; }
};
#elif WASMEDGE_OS_WINDOWS
static inline bool kSupported = true;
struct Implement {
  void *Address = nullptr;
  boost::winapi::HANDLE_ File = nullptr;
  boost::winapi::HANDLE_ Map = nullptr;
  Implement(const std::filesystem::path &Path) noexcept {
    File = boost::winapi::create_file(
        Path.native().c_str(), boost::winapi::GENERIC_READ_,
        boost::winapi::FILE_SHARE_READ_, nullptr, boost::winapi::OPEN_EXISTING_,
        boost::winapi::FILE_FLAG_SEQUENTIAL_SCAN_, nullptr);
    if (File == boost::winapi::invalid_handle_value) {
      File = nullptr;
      return;
    }

    boost::winapi::LARGE_INTEGER_ Size;
    boost::winapi::GetFileSizeEx(File, &Size);

    Map = boost::winapi::CreateFileMappingW(
        File, nullptr, boost::winapi::PAGE_READONLY_,
        static_cast<boost::winapi::ULONG_>(Size.HighPart), Size.LowPart,
        nullptr);
    if (Map == nullptr) {
      Map = boost::winapi::invalid_handle_value;
      return;
    }

    Address = boost::winapi::MapViewOfFile(Map, boost::winapi::FILE_MAP_READ_,
                                           0, 0, 0);
    if (Address == nullptr) {
      return;
    }
  }
  ~Implement() noexcept {
    if (Address) {
      boost::winapi::UnmapViewOfFile(Address);
    }
    if (Map) {
      boost::winapi::CloseHandle(Map);
    }
    if (File) {
      boost::winapi::CloseHandle(File);
    }
  }
  bool ok() const noexcept { return Address != nullptr; }
};
#else
static inline bool kSupported = false;
struct Implement {
  Implement(const std::filesystem::path &Path) noexcept = default;
  bool ok() const noexcept { return false; }
}
#endif
} // namespace

MMap::MMap(const std::filesystem::path &Path) noexcept : Handle(nullptr) {
  auto NativeHandle = std::make_unique<Implement>(Path);
  if (!NativeHandle->ok()) {
    return;
  }

  Handle = NativeHandle.release();
}

MMap::~MMap() noexcept {
  if (!Handle) {
    return;
  }

  std::unique_ptr<Implement> NativeHandle(
      reinterpret_cast<Implement *>(std::exchange(Handle, nullptr)));
}

void *MMap::address() const noexcept {
  if (!Handle) {
    return nullptr;
  }
  return reinterpret_cast<const Implement *>(Handle)->Address;
}

bool MMap::supported() noexcept { return kSupported; }

} // namespace WasmEdge

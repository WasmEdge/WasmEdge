// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include "system/winapi.h"
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
  winapi::HANDLE_ File = nullptr;
  winapi::HANDLE_ Map = nullptr;
  Implement(const std::filesystem::path &Path) noexcept {

#if NTDDI_VERSION >= NTDDI_WIN8
    winapi::CREATEFILE2_EXTENDED_PARAMETERS_ Create2ExParams;
    Create2ExParams.dwSize = sizeof(Create2ExParams);
    Create2ExParams.dwFileAttributes = 0;
    Create2ExParams.dwFileFlags = winapi::FILE_FLAG_SEQUENTIAL_SCAN_;
    Create2ExParams.dwSecurityQosFlags = 0;
    Create2ExParams.lpSecurityAttributes = nullptr;
    Create2ExParams.hTemplateFile = nullptr;

    File = winapi::CreateFile2(Path.c_str(), winapi::GENERIC_READ_,
                               winapi::FILE_SHARE_VALID_FLAGS_,
                               winapi::OPEN_EXISTING_, &Create2ExParams);
#else
    File = winapi::CreateFileW(Path.c_str(), winapi::GENERIC_READ_,
                               winapi::FILE_SHARE_VALID_FLAGS_, nullptr,
                               winapi::OPEN_EXISTING_,
                               winapi::FILE_FLAG_SEQUENTIAL_SCAN_, nullptr);
#endif
    if (File == winapi::INVALID_HANDLE_VALUE_) {
      File = nullptr;
      return;
    }

    winapi::LARGE_INTEGER_ Size;
    winapi::GetFileSizeEx(File, &Size);

#if NTDDI_VERSION >= NTDDI_WIN8
    Map = winapi::CreateFileMappingFromApp(
        File, nullptr, winapi::PAGE_READONLY_,
        static_cast<WasmEdge::winapi::ULONG64_>(Size.QuadPart), nullptr);
#else
    Map = winapi::CreateFileMappingW(File, nullptr, winapi::PAGE_READONLY_,
                                     static_cast<winapi::ULONG_>(Size.HighPart),
                                     Size.LowPart, nullptr);
#endif
    if (Map == nullptr) {
      return;
    }

#if NTDDI_VERSION >= NTDDI_WIN8
    Address = winapi::MapViewOfFileFromApp(Map, winapi::FILE_MAP_READ_, 0, 0);
#else
    Address = winapi::MapViewOfFile(Map, winapi::FILE_MAP_READ_, 0, 0, 0);
#endif
    if (Address == nullptr) {
      return;
    }
  }
  ~Implement() noexcept {
    if (Address) {
      winapi::UnmapViewOfFile(Address);
    }
    if (Map) {
      winapi::CloseHandle(Map);
    }
    if (File) {
      winapi::CloseHandle(File);
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

// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "macos.h"
#import <Foundation/Foundation.h>

namespace WasmEdge {
namespace Host {
namespace WASI {
inline namespace detail {

bool darwinAvailable(unsigned int Major, unsigned int Minor,
                     unsigned int Patch) noexcept {
  NSOperatingSystemVersion Version =
      NSProcessInfo.processInfo.operatingSystemVersion;
  if (Version.majorVersion < Major) {
    return false;
  }
  if (Version.majorVersion > Major) {
    return true;
  }

  if (Version.minorVersion < Minor) {
    return false;
  }
  if (Version.minorVersion > Minor) {
    return true;
  }

  if (Version.patchVersion < Patch) {
    return false;
  }
  return true;
}

} // namespace detail
} // namespace WASI
} // namespace Host
} // namespace WasmEdge

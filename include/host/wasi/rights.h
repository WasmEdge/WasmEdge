// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "wasi/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASI {

static inline constexpr const __wasi_rights_t kStdInDefaultRights =
    __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_FD_FILESTAT_GET |
    __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_POLL_FD_READWRITE;
static inline constexpr const __wasi_rights_t kStdOutDefaultRights =
    __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_FD_DATASYNC |
    __WASI_RIGHTS_FD_FILESTAT_GET | __WASI_RIGHTS_FD_SYNC |
    __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_POLL_FD_READWRITE;
static inline constexpr const __wasi_rights_t kStdErrDefaultRights =
    kStdOutDefaultRights;
static inline constexpr const __wasi_rights_t kNoInheritingRights =
    static_cast<__wasi_rights_t>(0);

} // namespace WASI
} // namespace Host
} // namespace WasmEdge

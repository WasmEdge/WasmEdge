// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2024 Second State INC

#include "api/vfs_io.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace WasmEdge {
namespace Host {
namespace API {

WasmEdgeIfstream::WasmEdgeIfstream(const Host::WASI::Environ &Env,
                                   const std::string_view &FileName) noexcept
    : Env_(const_cast<Host::WASI::Environ &>(Env)), Fd_(0), IsOpen(false),
      HasError(false), IsEof(false), Buffer_(4096), BufferPos_(0),
      BufferSize_(0) {

}

WasmEdgeIfstream::~WasmEdgeIfstream() {

}

WasmEdgeOfstream::WasmEdgeOfstream(const Host::WASI::Environ &Env,
                                   const std::string_view &FileName) noexcept
    : Env_(Env), Fd_(0), IsOpen(false), HasError(false), Buffer_(4096) {

}

WasmEdgeOfstream::~WasmEdgeOfstream() {

}

} // namespace API
} // namespace Host
} // namespace WasmEdge
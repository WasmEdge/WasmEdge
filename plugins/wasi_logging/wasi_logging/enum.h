#pragma once

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WASILOGGING {

enum WasiLoggingLevel : uint32_t { Trace, Debug, Info, Warn, Error, Critical };

} // namespace WASILOGGING
} // namespace Host
} // namespace WasmEdge
#pragma once

#include <utility>

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg{

enum class ErrNo : uint32_t {
  Success = 0,
  MissingMemory = 1,
  InvalidArgument = 2,
  RuntimeError = 3
};

}
}
}


#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize global segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::GlobalSegment &Seg,
                                  std::vector<uint8_t> &OutVec) {
  // Global segment: globaltype + expr.
  serializeType(Seg.getGlobalType(), OutVec);
  serializeExpression(Seg.getExpr(), OutVec);
}

// Serialize element segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::ElementSegment &,
                                  std::vector<uint8_t> &) {
  // Element segment: ...
  // TODO
}

// Serialize code segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::CodeSegment &,
                                  std::vector<uint8_t> &) {
  // Code segment: size:u32 + locals:vec(u32 + valtype) + body:expr.
  // TODO
}

// Serialize data segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::DataSegment &,
                                  std::vector<uint8_t> &) {
  // Data segment: ...
  // TODO
}

} // namespace Loader
} // namespace WasmEdge

#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize expression. See "include/loader/serialize.h".
void Serializer::serializeExpression(const AST::Expression &Expr,
                                     std::vector<uint8_t> &OutVec) {
  // Expression: instr*.
  for (const auto &Instr : Expr.getInstrs()) {
    serializeInstruction(Instr, OutVec);
  }
}

} // namespace Loader
} // namespace WasmEdge

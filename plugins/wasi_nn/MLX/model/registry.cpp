#include "registry.h"

namespace WasmEdge::Host::WASINN::MLX {
Transformer *llama38b(int VocabSize, float NormEps, float RopeTheta,
                      bool RopeTraditional) {
  return new Transformer(4096, std::vector<int>{14336}, VocabSize, 32,
                         std::vector<int>{32}, std::vector<int>{8}, NormEps, {},
                         RopeTraditional, RopeTheta);
}

Transformer *llama27bChat(int VocabSize, float NormEps, float RopeTheta,
                          bool RopeTraditional) {
  return new Transformer(4096, std::vector<int>{11008}, VocabSize, 32,
                         std::vector<int>{32}, std::vector<int>{32}, NormEps,
                         {}, RopeTraditional, RopeTheta);
}

Transformer *tinyLlama11BChatV10(int VocabSize, float NormEps, float RopeTheta,
                                 bool RopeTraditional) {
  return new Transformer(2048, std::vector<int>{5632}, VocabSize, 22,
                         std::vector<int>{32}, std::vector<int>{4}, NormEps, {},
                         RopeTraditional, RopeTheta);
}
} // namespace WasmEdge::Host::WASINN::MLX

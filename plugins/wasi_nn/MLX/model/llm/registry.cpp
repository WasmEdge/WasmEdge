// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "registry.h"
#include "transformer.h"
namespace WasmEdge::Host::WASINN::MLX {
namespace llm {
std::shared_ptr<Transformer> llama38b(int VocabSize, float NormEps,
                                      float RopeTheta, bool RopeTraditional) {
  return std::make_shared<Transformer>(Transformer(
      4096, std::vector<int>{14336}, VocabSize, 32, std::vector<int>{32},
      std::vector<int>{8}, NormEps, {}, RopeTraditional, RopeTheta));
}

std::shared_ptr<Transformer> llama27bChat(int VocabSize, float NormEps,
                                          float RopeTheta,
                                          bool RopeTraditional) {
  return std::make_shared<Transformer>(Transformer(
      4096, std::vector<int>{11008}, VocabSize, 32, std::vector<int>{32},
      std::vector<int>{32}, NormEps, {}, RopeTraditional, RopeTheta));
}

std::shared_ptr<Transformer> tinyLlama11BChatV10(int VocabSize, float NormEps,
                                                 float RopeTheta,
                                                 bool RopeTraditional) {
  return std::make_shared<Transformer>(Transformer(
      2048, std::vector<int>{5632}, VocabSize, 22, std::vector<int>{32},
      std::vector<int>{4}, NormEps, {}, RopeTraditional, RopeTheta));
}

} // namespace llm
} // namespace WasmEdge::Host::WASINN::MLX

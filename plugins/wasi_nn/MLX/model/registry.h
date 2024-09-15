#pragma once

#include "transformer.h"

namespace WasmEdge::Host::WASINN::MLX {
Transformer *llama38b(int VocabSize = 32000, float NormEps = 1e-5,
                      float RopeTheta = 10000.0, bool RopeTraditional = false);

Transformer *llama27bChat(int VocabSize = 32000, float NormEps = 1e-5,
                          float RopeTheta = 10000.0,
                          bool RopeTraditional = false);

Transformer *tinyLlama11BChatV10(int VocabSize = 32000, float NormEps = 1e-5,
                                 float RopeTheta = 10000.0,
                                 bool RopeTraditional = false);
} // namespace WasmEdge::Host::WASINN::MLX

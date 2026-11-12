// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "transformer.h"
namespace WasmEdge::Host::WASINN::MLX {
namespace llm {
std::shared_ptr<Transformer> llama38b(int VocabSize = 32000,
                                      float NormEps = 1e-5,
                                      float RopeTheta = 10000.0,
                                      bool RopeTraditional = false);

std::shared_ptr<Transformer> llama27bChat(int VocabSize = 32000,
                                          float NormEps = 1e-5,
                                          float RopeTheta = 10000.0,
                                          bool RopeTraditional = false);

std::shared_ptr<Transformer> tinyLlama11BChatV10(int VocabSize = 32000,
                                                 float NormEps = 1e-5,
                                                 float RopeTheta = 10000.0,
                                                 bool RopeTraditional = false);

} // namespace llm
} // namespace WasmEdge::Host::WASINN::MLX

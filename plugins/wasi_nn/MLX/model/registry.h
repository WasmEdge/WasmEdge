// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "model/transformer.h"

namespace WasmEdge::Host::WASINN::MLX {

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

} // namespace WasmEdge::Host::WASINN::MLX

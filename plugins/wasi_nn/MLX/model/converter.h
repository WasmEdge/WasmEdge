// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "mlx/base.h"

#include <mlx/mlx.h>

#include <string>
#include <unordered_map>

namespace WasmEdge::Host::WASINN::MLX {

#define strReplace(Str, From, To) Str.replace(Str.find(From), strlen(From), To)

std::unordered_map<std::string, mx::array> weightsToMlx(std::string WeightPath);

std::unordered_map<std::string, mx::array>
llamaToMlxllm(std::string WeightPath);

} // namespace WasmEdge::Host::WASINN::MLX

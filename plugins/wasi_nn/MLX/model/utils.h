// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "mlx/base.h"

#include <iostream>
#include <string>
#include <vector>

namespace WasmEdge::Host::WASINN::MLX {

std::vector<std::string> splitString(const std::string &S, char Delim);

std::string joinString(std::vector<std::string> &S, char Delim);

bool endsWith(std::string const &Value, std::string const &Ending);

bool startsWith(std::string const &Value, std::string const &Starting);

void saveWeights(const std::unordered_map<std::string, mx::array> &Weights,
                 const std::string Path);

void saveWeights(const mx::array &Weights, const std::string &Path);

} // namespace WasmEdge::Host::WASINN::MLX

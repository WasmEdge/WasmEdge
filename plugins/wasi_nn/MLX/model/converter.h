#pragma once
#include "base.h"
#include "mlx/mlx.h"
#include <cstring>

#define strReplace(Str, From, To) Str.replace(Str.find(From), strlen(From), To)

std::unordered_map<std::string, mx::array> weightsToMlx(std::string WeightPath);

std::unordered_map<std::string, mx::array>
llamaToMlxllm(std::string WeightPath);
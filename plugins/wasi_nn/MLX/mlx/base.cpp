// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "mlx/base.h"
#include "model/utils.h"

#include <mlx/array.h>

namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {

mx::array &Module::registerParameter(std::string Name, mx::array &&W) {
  Parameters.insert({Name, W});
  return Parameters.at(Name);
}

void Module::update(std::unordered_map<std::string, mx::array> Parameters) {
  for (auto &[K, V] : Parameters) {
    apply(K, V);
  }
}

std::shared_ptr<nn::Module> Module::toQuantized(
    int GroupSize, int Bits, const std::string &Prefix,
    const std::unordered_map<std::string, mx::array> &Parameters) {
  auto NewPrefix = Prefix + Name + (Prefix.empty() && Name.empty() ? "" : ".");
  for (auto &[K, V] : Submodules) {
    if (V->hasQuantize()) {
      auto Weights = V->Parameters.find("weight");
      if (Weights != V->Parameters.end() && !Parameters.empty()) {
        if (Parameters.count(NewPrefix + V->Name + ".scales") == 0) {
          continue;
        }
      }
      if (Weights != V->Parameters.end() &&
          Weights->second.shape().back() % GroupSize != 0) {
        continue;
      }
    }
    V = V->toQuantized(GroupSize, Bits,
                       Prefix + Name + (Name.empty() ? "" : "."), Parameters);
  }
  return shared_from_this();
}

void Module::apply(std::string Key, mx::array Value) {
  std::vector<std::string> SplitKey = splitString(Key, '.');
  if (SplitKey.size() == 1) {
    if (Parameters.find(Key) == Parameters.end()) {
      spdlog::error("[WASI-NN] MLX backend: Unsupported weight: {}"sv, Key);
      assumingUnreachable();
    }
    this->Parameters.at(Key) = Value;
  } else {
    std::string LayerName = SplitKey[0];
    SplitKey.erase(SplitKey.begin());
    if (LayerName == "layers" || LayerName == "blocks") {
      LayerName += "." + SplitKey[0];
      SplitKey.erase(SplitKey.begin());
    }
    if (Submodules.find(LayerName) == Submodules.end()) {
      spdlog::error("[WASI-NN] MLX backend: Unsupported Layer: {}"sv,
                    LayerName);
      assumingUnreachable();
    }
    Submodules.at(LayerName)->apply(joinString(SplitKey, '.'), Value);
  }
}

std::unordered_map<std::string, mx::array>
Module::getWeigts(const std::string &Prefix) {
  std::unordered_map<std::string, mx::array> Weights;
  auto NewPrefix = Prefix + Name;
  for (auto &[K, V] : Submodules) {
    auto Subweights = V->getWeigts(NewPrefix + (NewPrefix.empty() ? "" : "."));
    Weights.insert(Subweights.begin(), Subweights.end());
  }
  for (auto &[K, V] : Parameters) {
    Weights.insert({NewPrefix + (NewPrefix.empty() ? "" : ".") + K, V});
  }
  return Weights;
}

} // namespace mlx::core::nn
} // namespace WasmEdge::Host::WASINN::MLX

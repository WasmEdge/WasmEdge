#pragma once

#include "common/errcode.h"
#include "mlx/mlx.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace mx = mlx::core;
namespace WasmEdge::Host::WASINN::MLX {
namespace mlx::core::nn {
class Module {
public:
  virtual ~Module() {
    for (auto Module : Submodules) {
      delete Module.second;
    }
  }
  std::string Name;
  std::unordered_map<std::string, mx::array> Parameters{};
  std::unordered_map<std::string, Module *> Submodules{};
  mx::array &registerParameter(std::string Name, mx::array &&W);
  std::unordered_map<std::string, mx::array>
  getWeigts(const std::string &Prefix = "model");
  virtual nn::Module *toQuantized(int GroupSize = 64, int Bits = 4);
  void update(std::unordered_map<std::string, mx::array> Parameters);
  void apply(std::string Key, mx::array Parameters);
  template <typename T> void registerModule(std::string ModuleName, T *M) {
    using DecayedT = std::decay_t<T>;
    if (!std::is_base_of<Module, DecayedT>::value) {
      spdlog::error("Invalid subModule.");
      assumingUnreachable();
    }

    if (Submodules.find(ModuleName) == Submodules.end()) {
      Submodules.insert({ModuleName, M});
      Submodules.at(ModuleName)->Name = ModuleName;
    } else {
      spdlog::error("Module already exists.");
      assumingUnreachable();
    }
  }
  template <typename T>
  void registerLayer(std::string ModuleName, std::vector<T *> &Layers) {
    if (!std::is_base_of<Module, T>::value) {
      spdlog::error("Invalid subModule.");
      assumingUnreachable();
    }
    for (size_t Idx = 0; Idx < Layers.size(); Idx++) {
      registerModule(ModuleName + "." + std::to_string(Idx), Layers[Idx]);
    }
  }
};
template <typename T> void printVec(std::vector<T> Ve) {
  for (auto I : Ve) {
    spdlog::debug("{} ", I);
  }
}
} // namespace mlx::core::nn

} // namespace WasmEdge::Host::WASINN::MLX

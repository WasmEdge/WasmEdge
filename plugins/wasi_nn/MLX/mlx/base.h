#pragma once

#include "mlx/mlx.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace mx = mlx::core;

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
  mx::array &registerParameter(std::string Name, array &&W);

  void update(std::unordered_map<std::string, mx::array> Parameters);
  void apply(std::string Key, mx::array Parameters);
  template <typename T> void registerModule(std::string ModuleName, T *M) {
    using DecayedT = std::decay_t<T>;
    if (!std::is_base_of<Module, DecayedT>::value) {
      throw std::invalid_argument("Invalid subModule.");
    }

    if (Submodules.find(ModuleName) == Submodules.end()) {
      Submodules.insert({ModuleName, M});
      Submodules.at(ModuleName)->Name = ModuleName;
    }
  }
  template <typename T>
  void registerLayer(std::string ModuleName, std::vector<T *> &Layers) {
    if (!std::is_base_of<Module, T>::value) {
      throw std::invalid_argument("Invalid subModule.");
    }
    for (size_t Idx = 0; Idx < Layers.size(); Idx++) {
      registerModule(ModuleName + "." + std::to_string(Idx), Layers[Idx]);
    }
  }
};
} // namespace mlx::core::nn

template <typename T> void printVec(std::vector<T> Ve) {
  for (auto I : Ve) {
    std::cout << I << " ";
  }
  std::cout << std::endl;
}
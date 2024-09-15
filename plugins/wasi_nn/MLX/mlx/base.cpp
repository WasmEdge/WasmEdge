#include "base.h"
#include "../model/utils.h"
#include <mlx/array.h>
#include <unordered_map>

namespace mlx::core::nn {

mx::array &Module::registerParameter(std::string Name, array &&W) {
  Parameters.insert({Name, W});
  return Parameters.at(Name);
}
void Module::update(std::unordered_map<std::string, mx::array> Parameters) {
  for (auto &[k, v] : Parameters) {
    apply(k, v);
  }
}
nn::Module *Module::toQuantized(int GroupSize, int Bits) {
  for (auto &[k, v] : Submodules) {
    auto *OldModule = v;
    v = v->toQuantized(GroupSize, Bits);
    if (OldModule != v) {
      delete OldModule;
    }
  }
  return this;
}
void Module::apply(std::string Key, mx::array Value) {
  std::vector<std::string> SplitKey = splitString(Key, '.');
  if (SplitKey.size() == 1) {
    if (Parameters.find(Key) == Parameters.end()) {
      spdlog::error("Unsupported weight: {}", Key);
      assumingUnreachable();
    }
    this->Parameters.at(Key) = Value;
  } else {
    std::string LayerName = SplitKey[0];
    SplitKey.erase(SplitKey.begin());
    if (LayerName == "layers") {
      LayerName += "." + SplitKey[0];
      SplitKey.erase(SplitKey.begin());
    }
    if (Submodules.find(LayerName) == Submodules.end()) {
      spdlog::error("Unsupported Layer: {}", LayerName);
      assumingUnreachable();
    }
    Submodules.at(LayerName)->apply(joinString(SplitKey, '.'), Value);
  }
}
std::unordered_map<std::string, mx::array>
Module::getWeigts(const std::string &Prefix) {
  std::unordered_map<std::string, mx::array> Weights;
  for (auto &[k, v] : Submodules) {
    auto Subweights = v->getWeigts(Prefix + Name + ".");
    Weights.insert(Subweights.begin(), Subweights.end());
  }
  for (auto &[k, v] : Parameters) {
    Weights.insert({Prefix + Name + "." + k, v});
  }
  return Weights;
}
} // namespace mlx::core::nn
#include "base.h"
#include "../model/utils.h"
#include <mlx/array.h>

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
void Module::apply(std::string Key, mx::array Value) {
  std::vector<std::string> SplitKey = splitString(Key, '.');
  if (SplitKey.size() == 1) {
    if (Parameters.find(Key) == Parameters.end()) {
      throw std::invalid_argument("Unsupported weight: " + Key);
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
      throw std::invalid_argument("Unsupported Tensor: " + LayerName);
    }
    Submodules.at(LayerName)->apply(joinString(SplitKey, '.'), Value);
  }
}
} // namespace mlx::core::nn
#include "converter.h"
#include "utils.h"
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace WasmEdge::Host::WASINN::MLX {
std::unordered_map<std::string, mx::array>
weightsToMlx(std::string WeightPath) {
  const std::filesystem::path Path(WeightPath);
  if (std::filesystem::is_directory(Path)) {
    std::unordered_map<std::string, mx::array> Loaded;
    for (const auto &Entry : std::filesystem::directory_iterator(Path)) {
      if (Entry.path().extension() == ".safetensors") {
        auto SubWeight = weightsToMlx(Entry.path());
        Loaded.insert(SubWeight.begin(), SubWeight.end());
      }
    }
    return Loaded;
  }
  if (endsWith(WeightPath, ".safetensors")) {
    spdlog::info("Loading model from .safetensors file...\n");
    const mx::SafetensorsLoad Loaded = mx::load_safetensors(WeightPath);
    return Loaded.first;
  }
  if (endsWith(WeightPath, ".gguf")) {
    spdlog::info("Loading model from .gguf file...\n");
    const mx::GGUFLoad Loaded = mx::load_gguf(WeightPath);
    return Loaded.first;
  }
  spdlog::error("Can not regonize model file\n");
  assumingUnreachable();
}

std::unordered_map<std::string, mx::array>
llamaToMlxllm(std::string WeightPath) {
  std::unordered_map<std::string, mx::array> ModelWeights;
  auto Weight = weightsToMlx(WeightPath);
  for (auto &[k, v] : Weight) {
    std::string NewKey = k;
    if (startsWith(NewKey, "model.")) {
      strReplace(NewKey, "model.", "");
    }
    std::vector<std::string> SplitKey = splitString(NewKey, '.');
    if (find(SplitKey.begin(), SplitKey.end(), "layers") != SplitKey.end()) {
      if (find(SplitKey.begin(), SplitKey.end(), "rotary_emb") !=
          SplitKey.end()) {
        continue;
      }
      if (find(SplitKey.begin(), SplitKey.end(), "self_attn") !=
          SplitKey.end()) {
        ModelWeights.insert({SplitKey[0] + "." + SplitKey[1] + ".attention." +
                                 SplitKey[3] + "." + SplitKey[4],
                             v});
      } else if (find(SplitKey.begin(), SplitKey.end(), "mlp") !=
                 SplitKey.end()) {

        ModelWeights.insert({NewKey, v});
      } else {
        const std::unordered_map<std::string, std::string> KeyMap = {
            {"input_layernorm", "attention_norm"},
            {"post_attention_layernorm", "mlp_norm"}};
        if (KeyMap.find(SplitKey[2]) == KeyMap.end()) {
          ModelWeights.insert({NewKey, v});
        } else {
          ModelWeights.insert({SplitKey[0] + "." + SplitKey[1] + "." +
                                   KeyMap.at(SplitKey[2]) + "." + SplitKey[3],
                               v});
        }
      }
    } else {
      const std::unordered_map<std::string, std::string> KeyMap = {
          {"embed_tokens", "token_embed"},
          {"lm_head", "head"},
          {"norm", "norm"}};
      if (KeyMap.find(SplitKey[0]) == KeyMap.end()) {
        ModelWeights.insert({NewKey, v});
      } else {
        ModelWeights.insert({KeyMap.at(SplitKey[0]) + "." + SplitKey[1], v});
      }
    }
  }
  return ModelWeights;
}
} // namespace WasmEdge::Host::WASINN::MLX

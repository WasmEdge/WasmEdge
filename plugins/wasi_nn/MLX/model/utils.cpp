#include "utils.h"
#include <sstream>
std::vector<std::string> splitString(const std::string &S, char Delim) {
  std::vector<std::string> Result;
  std::stringstream SS(S);
  std::string Item;
  while (std::getline(SS, Item, Delim)) {
    Result.emplace_back(Item);
  }
  return Result;
}
std::string joinString(std::vector<std::string> &S, char Delim) {
  std::string Result;
  for (size_t Idx = 0; Idx < S.size(); Idx++) {
    Result += S[Idx];
    if (Idx != S.size() - 1) {
      Result += Delim;
    }
  }
  return Result;
}

bool endsWith(std::string const &Value, std::string const &Ending) {
  if (Ending.size() > Value.size())
    return false;
  return std::equal(Ending.rbegin(), Ending.rend(), Value.rbegin());
}
bool startsWith(std::string const &Value, std::string const &Starting) {
  if (Starting.size() > Value.size())
    return false;
  return std::equal(Starting.begin(), Starting.end(), Value.begin());
}
void saveWeights(const std::unordered_map<std::string, mx::array> &Weights,
                 const std::string Path) {
  if (endsWith(Path, ".safetensors")) {
    mx::save_safetensors(Path, Weights, {{"format", "mlx"}});
  } else {
    spdlog::error("Unsupported file format");
    assumingUnreachable();
  }
}
void saveWeights(const mx::array &Weights, const std::string &Path) {
  if (endsWith(Path, ".npz")) {
    mx::save(Path, Weights);
  } else {
    spdlog::error("Unsupported file format");
    assumingUnreachable();
  }
}
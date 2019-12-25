#pragma once

#include "rapidjson/document.h"
#include "vm/vm.h"
#include <memory>
#include <string>
#include <vector>

namespace SSVM {
namespace Proxy {

class Proxy {
public:
  Proxy() = default;
  ~Proxy() = default;

  void setInputJSONPath(const std::string &S) { InputJSONPath = S; }
  void setOutputJSONPath(const std::string &S) { OutputJSONPath = S; }
  void setWasmPath(const std::string &S) { WasmPath = S; }

  const std::string &getInputJSONPath() { return InputJSONPath; }
  const std::string &getOutputJSONPath() { return OutputJSONPath; }
  const std::string &getWasmPath() { return WasmPath; }

  void runRequest();

private:
  void prepareOutputJSON();
  void parseInputJSON();
  void executeVM();
  void exportOutputJSON();

  std::string InputJSONPath;
  std::string OutputJSONPath;
  std::string WasmPath;
  rapidjson::Document InputDoc;
  rapidjson::Document OutputDoc;
  std::unique_ptr<VM::VM> VMUnit;
};

} // namespace Proxy
} // namespace SSVM

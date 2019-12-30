// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "rapidjson/document.h"
#include "vm/configure.h"
#include "vm/vm.h"

#include <boost/filesystem.hpp>
#include <memory>
#include <string>
#include <vector>

namespace SSVM {
namespace Proxy {

namespace {

std::string getAbsPath(const std::string &RelPath) {
  if (RelPath == "") {
    return RelPath;
  }
  boost::filesystem::path Path;
  if (RelPath[0] == '~') {
    const char *Home = getenv("HOME");
    if (Home != NULL) {
      Path = std::string(Home) + '/' + RelPath.substr(2);
    }
  } else {
    Path = RelPath;
  }
  return boost::filesystem::weakly_canonical(Path).string();
}

} // namespace

class Proxy {
public:
  Proxy() = default;
  ~Proxy() = default;

  void setInputJSONPath(const std::string &S) { InputJSONPath = getAbsPath(S); }
  void setOutputJSONPath(const std::string &S) {
    OutputJSONPath = getAbsPath(S);
  }
  void setWasmPath(const std::string &S) { WasmPath = getAbsPath(S); }

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
  VM::Configure VMConf;
};

} // namespace Proxy
} // namespace SSVM

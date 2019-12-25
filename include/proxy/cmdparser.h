#pragma once

#include <string>

namespace SSVM {
namespace Proxy {

/// TODO: Use tool to parse command line arguments.
class CmdParser {
public:
  void parseCommandLine(int Argc, const char *const *Argv);
  void printHelperMsg();
  const std::string &getInputJSONPath() { return InputJSONPath; }
  const std::string &getOutputJSONPath() { return OutputJSONPath; }
  const std::string &getWasmPath() { return WasmPath; }

private:
  std::string InputJSONPath;
  std::string OutputJSONPath;
  std::string WasmPath;
};

} // namespace Proxy
} // namespace SSVM

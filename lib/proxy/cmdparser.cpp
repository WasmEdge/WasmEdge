// SPDX-License-Identifier: Apache-2.0
#include "proxy/cmdparser.h"
#include <iostream>
#include <string>

namespace SSVM {
namespace Proxy {

void CmdParser::printHelperMsg() {
  std::cout << "Usage:\n"
            << "  --input_file=<path-of-input-JSON-file>\n"
            << "  --output_file=<path-of-output-JSON-file>\n"
            << "  --wasm_file=<path-of-wasm-file>\n";
}

void CmdParser::parseCommandLine(int Argc, const char *const *Argv) {
  for (unsigned int I = 0; I < Argc; ++I) {
    std::string S(Argv[I]);

    // Get the Input JSON file path.
    if (S.rfind("--input_file=") == 0) {
      InputJSONPath = S.substr(13);
    }

    // Get the Output JSON file path.
    if (S.rfind("--output_file=") == 0) {
      OutputJSONPath = S.substr(14);
    }

    // Get the bytecode file path.
    if (S.rfind("--wasm_file=") == 0) {
      WasmPath = S.substr(12);
    }
  }
}

} // namespace Proxy
} // namespace SSVM

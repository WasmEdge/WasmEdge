// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include <filesystem>

#include "common/defines.h"
#include "common/errcode.h"
#include "func.h"

namespace WasmEdge {
namespace Host {

Expect<List<Tuple<std::string, std::string>>> GetEnvironment::body() {
  List<Tuple<std::string, std::string>> L{};

  // TODO: people figure out can use
  // https://stackoverflow.com/questions/16765545/how-to-list-all-environment-variables-in-a-c-c-app
  //
  //    extern char **environ;
  //
  // to get all environment variables, but didn't say the portability

  return std::move(L);
}

Expect<List<std::string>> GetArguments::body() {
  List<std::string> L{};
  // TODO: figure out how to pass this
  return std::move(L);
}

Expect<std::string> InitialCwd::body() {
  std::filesystem::path CWD = std::filesystem::current_path();
  std::string S{CWD.string()};
  return std::move(S);
}

Expect<void> Exit::body() {
  exit(0);
  return {};
}
Expect<void> ExitWithCode::body(uint8_t StatusCode) {
  exit(StatusCode);
  return {};
}

} // namespace Host
} // namespace WasmEdge

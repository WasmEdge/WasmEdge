// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#ifdef WASMEDGE_BUILD_FUZZING
#include "driver/fuzzWAT.h"
#include "common/configure.h"
#include "loader/serialize.h"
#include "validator/validator.h"
#include "wat/parser.h"

namespace WasmEdge {
namespace Driver {

int FuzzWAT(const uint8_t *Data, size_t Size) noexcept {
  std::ios::sync_with_stdio(false);
  spdlog::set_level(spdlog::level::critical);

  // The default Configure holds the WASM 3.0 proposals. The fuzzer adds the
  // proposals that are off by default, so that every syntax of the grammar
  // gets to the converter.
  Configure Conf;
  Conf.addProposal(Proposal::Annotations);
  Conf.addProposal(Proposal::Threads);
  // This is the WAT fuzzer, so the experimental text format input is on.
  Conf.setEnableWAT(true);

  std::string_view Source(reinterpret_cast<const char *>(Data), Size);

  auto ParseRes = WAT::parseWat(Source, Conf);
  if (!ParseRes) {
    return 0;
  }

  Validator::Validator ValidatorEngine(Conf);
  if (auto Res = ValidatorEngine.validate(*ParseRes); !Res) {
    return 0;
  }

  Loader::Serializer Serializer;
  if (auto Res = Serializer.serializeModule(*ParseRes); !Res) {
    return 0;
  }

  return 0;
}

} // namespace Driver
} // namespace WasmEdge
#endif

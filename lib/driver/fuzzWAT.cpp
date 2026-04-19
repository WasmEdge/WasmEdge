// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

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

  Configure Conf;
  Conf.addProposal(Proposal::TailCall);
  Conf.addProposal(Proposal::ExtendedConst);
  Conf.addProposal(Proposal::FunctionReferences);
  Conf.addProposal(Proposal::GC);
  Conf.addProposal(Proposal::MultiMemories);
  Conf.addProposal(Proposal::RelaxSIMD);
  Conf.addProposal(Proposal::Annotations);
  Conf.addProposal(Proposal::ExceptionHandling);
  Conf.addProposal(Proposal::Memory64);
  Conf.addProposal(Proposal::Threads);
  Conf.addProposal(Proposal::Component);

  std::string_view Source(reinterpret_cast<const char *>(Data), Size);

  auto ParseRes = WAT::parseWat(Source, Conf);
  if (!ParseRes) {
    return 0;
  }

  Validator::Validator ValidatorEngine(Conf);
  if (auto Res = ValidatorEngine.validate(*ParseRes); !Res) {
    return 0;
  }

  Loader::Serializer Serializer(Conf);
  if (auto Res = Serializer.serializeModule(*ParseRes); !Res) {
    return 0;
  }

  return 0;
}

} // namespace Driver
} // namespace WasmEdge
#endif

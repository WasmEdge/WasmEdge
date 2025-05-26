// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

using namespace std::literals;

namespace WasmEdge {
namespace Loader {

Expect<std::pair<std::vector<Byte>, std::vector<Byte>>> Loader::loadPreamble() {
  // component ::= <preamble> s*:<section>* => (component flatten(s*))
  // preamble  ::= <magic> <version> <layer>
  // magic     ::= 0x00 0x61 0x73 0x6D
  // version   ::= 0x0a 0x00
  // layer     ::= 0x01 0x00

  // The combination of version and layer is corresponding to the version of
  // core wasm.
  // The core module has same magic but the different version:
  // 0x01 0x00 0x00 0x00
  auto Magic = FMgr.readBytes(4);
  if (!Magic) {
    return logLoadError(Magic.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  std::vector<Byte> WasmMagic = {0x00, 0x61, 0x73, 0x6D};
  if (*Magic != WasmMagic) {
    auto M = *Magic;
    spdlog::error("Might an invalid wasm file, magic expected, but got 0x{:X} "
                  "0x{:X} 0x{:X} 0x{:X}",
                  M[0], M[1], M[2], M[3]);
    return logLoadError(ErrCode::Value::MalformedMagic, FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  auto Ver = FMgr.readBytes(4);
  if (!Ver) {
    return logLoadError(Ver.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  return std::make_pair(*Magic, *Ver);
}

Expect<void> Loader::loadComponent(AST::Component::Component &Comp,
                                   std::optional<uint64_t> Bound) {
  using namespace AST::Component;

  uint64_t StartOffset = FMgr.getOffset();
  uint64_t Offset = FMgr.getOffset();
  Expect<Byte> ResSecId;

  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return E;
  };

  while ((!Bound.has_value() || *Bound > Offset - StartOffset) &&
         (ResSecId = FMgr.readByte())) {
    if (!ResSecId) {
      return logLoadError(ResSecId.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    }
    // keep going only if we have new section ID
    uint8_t NewSectionId = *ResSecId;

    switch (NewSectionId) {
    case 0x00:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Comp.getSections().back().emplace<AST::CustomSection>())
              .map_error(ReportError));
      break;
    case 0x01:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Comp.getSections().back().emplace<CoreModuleSection>())
              .map_error(ReportError));
      break;
    case 0x02:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Comp.getSections().back().emplace<CoreInstanceSection>())
              .map_error(ReportError));
      break;
    case 0x03:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Comp.getSections().back().emplace<CoreTypeSection>())
              .map_error(ReportError));
      break;
    case 0x04:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Comp.getSections().back().emplace<ComponentSection>())
              .map_error(ReportError));
      break;
    case 0x05:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Comp.getSections().back().emplace<InstanceSection>())
              .map_error(ReportError));
      break;
    case 0x06:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Comp.getSections().back().emplace<AliasSection>())
              .map_error(ReportError));
      break;
    case 0x07:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(loadSection(Comp.getSections().back().emplace<TypeSection>())
                       .map_error(ReportError));
      break;
    case 0x08:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Comp.getSections().back().emplace<CanonSection>())
              .map_error(ReportError));
      break;
    case 0x09:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Comp.getSections().back().emplace<StartSection>())
              .map_error(ReportError));
      break;
    case 0x0A:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Comp.getSections().back().emplace<ImportSection>())
              .map_error(ReportError));
      break;
    case 0x0B:
      Comp.getSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Comp.getSections().back().emplace<ExportSection>())
              .map_error(ReportError));
      break;
    default:
      return logLoadError(ErrCode::Value::MalformedSection,
                          FMgr.getLastOffset(), ASTNodeAttr::Component);
    }
    Offset = FMgr.getOffset();
  }

  return {};
}

} // namespace Loader
} // namespace WasmEdge

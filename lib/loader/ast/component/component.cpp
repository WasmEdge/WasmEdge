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
                        ASTNodeAttr::Module);
  }
  std::vector<Byte> WasmMagic = {0x00, 0x61, 0x73, 0x6D};
  if (*Magic != WasmMagic) {
    auto M = *Magic;
    spdlog::error("Might an invalid wasm file, magic expected, but got 0x{:X} "
                  "0x{:X} 0x{:X} 0x{:X}"sv,
                  M[0], M[1], M[2], M[3]);
    return logLoadError(ErrCode::Value::MalformedMagic, FMgr.getLastOffset(),
                        ASTNodeAttr::Module);
  }
  auto Ver = FMgr.readBytes(4);
  if (!Ver) {
    return logLoadError(Ver.error(), FMgr.getLastOffset(), ASTNodeAttr::Module);
  }
  return std::make_pair(*Magic, *Ver);
}

Expect<void> Loader::loadComponent(AST::Component::Component &Comp,
                                   std::optional<uint64_t> Bound) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return E;
  };
  // component ::= <preamble> s*:<section>*           => (component flatten(s*))
  // section   ::=    section_0(<core:custom>)        => ϵ
  //             | m: section_1(<core:module>)        => [core-prefix(m)]
  //             | i*:section_2(vec(<core:instance>)) => core-prefix(i)*
  //             | t*:section_3(vec(<core:type>))     => core-prefix(t)*
  //             | c: section_4(<component>)          => [c]
  //             | i*:section_5(vec(<instance>))      => i*
  //             | a*:section_6(vec(<alias>))         => a*
  //             | t*:section_7(vec(<type>))          => t*
  //             | c*:section_8(vec(<canon>))         => c*
  //             | s: section_9(<start>)              => [s]
  //             | i*:section_10(vec(<import>))       => i*
  //             | e*:section_11(vec(<export>))       => e*
  //             | v*:section_12(vec(<value>))        => v* 🪙
  uint64_t StartOffset = FMgr.getOffset();
  uint64_t Offset = FMgr.getOffset();
  Expect<Byte> ResSecId;

  while ((!Bound.has_value() || *Bound > Offset - StartOffset) &&
         (ResSecId = FMgr.readByte())) {
    if (!ResSecId) {
      return logLoadError(ResSecId.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    }
    // keep going only if we have new section ID
    uint8_t NewSectionId = *ResSecId;
    Comp.getSections().emplace_back();
    auto &Sec = Comp.getSections().back();

    switch (NewSectionId) {
    case 0x00:
      EXPECTED_TRY(loadSection(Sec.emplace<AST::CustomSection>())
                       .map_error(ReportError));
      break;
    case 0x01:
      EXPECTED_TRY(loadSection(Sec.emplace<AST::Component::CoreModuleSection>())
                       .map_error(ReportError));
      break;
    case 0x02:
      EXPECTED_TRY(
          loadSection(Sec.emplace<AST::Component::CoreInstanceSection>())
              .map_error(ReportError));
      break;
    case 0x03:
      EXPECTED_TRY(loadSection(Sec.emplace<AST::Component::CoreTypeSection>())
                       .map_error(ReportError));
      break;
    case 0x04:
      EXPECTED_TRY(loadSection(Sec.emplace<AST::Component::ComponentSection>())
                       .map_error(ReportError));
      break;
    case 0x05:
      EXPECTED_TRY(loadSection(Sec.emplace<AST::Component::InstanceSection>())
                       .map_error(ReportError));
      break;
    case 0x06:
      EXPECTED_TRY(loadSection(Sec.emplace<AST::Component::AliasSection>())
                       .map_error(ReportError));
      break;
    case 0x07:
      EXPECTED_TRY(loadSection(Sec.emplace<AST::Component::TypeSection>())
                       .map_error(ReportError));
      break;
    case 0x08:
      EXPECTED_TRY(loadSection(Sec.emplace<AST::Component::CanonSection>())
                       .map_error(ReportError));
      break;
    case 0x09:
      EXPECTED_TRY(loadSection(Sec.emplace<AST::Component::StartSection>())
                       .map_error(ReportError));
      break;
    case 0x0A:
      EXPECTED_TRY(loadSection(Sec.emplace<AST::Component::ImportSection>())
                       .map_error(ReportError));
      break;
    case 0x0B:
      EXPECTED_TRY(loadSection(Sec.emplace<AST::Component::ExportSection>())
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

// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/spec/spectest.cpp - Wasm test suites ----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file parse and run tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/testsuite
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "spectest.h"
#include "common/errcode.h"
#include "common/hash.h"
#include "common/spdlog.h"
#include "json_parser.h"
#include "wast_parser.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iterator>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <variant>

namespace {

using namespace std::literals;
using namespace WasmEdge;

enum class V128Shape { I8x16, I16x8, I32x4, I64x2, F32x4, F64x2, Unknown };
static const auto &V128ShapeMap() {
  static const std::unordered_map<std::string_view, V128Shape, Hash::Hash> Map =
      {
          {"i8x16"sv, V128Shape::I8x16}, {"i16x8"sv, V128Shape::I16x8},
          {"i32x4"sv, V128Shape::I32x4}, {"i64x2"sv, V128Shape::I64x2},
          {"f32x4"sv, V128Shape::F32x4}, {"f64x2"sv, V128Shape::F64x2},
      };
  return Map;
}

enum class LaneKind { F32, F64, I8, I16, I32, I64, Unknown };

struct TestsuiteProposal {
  TestsuiteProposal(
      std::string_view P,
      const WasmEdge::Standard Std = WasmEdge::Standard::WASM_3,
      const std::vector<WasmEdge::Proposal> &EnableProps = {},
      const std::vector<WasmEdge::Proposal> &DisableProps = {},
      WasmEdge::SpecTest::TestMode M = WasmEdge::SpecTest::TestMode::All)
      : Path(P), Mode(M) {
    Conf.setWASMStandard(Std);
    for (const auto &Prop : EnableProps) {
      Conf.addProposal(Prop);
    }
    for (const auto &Prop : DisableProps) {
      Conf.removeProposal(Prop);
    }
  }

  std::string_view Path;
  WasmEdge::Configure Conf;
  WasmEdge::SpecTest::TestMode Mode = WasmEdge::SpecTest::TestMode::All;
};

static const TestsuiteProposal TestsuiteProposals[] = {
    // | Folder | WASM_Base | Additional_set | Removal_set | Mode |
    // ------------------------------------------------------------
    // Folder: the directory name of tests.
    // WASM_Base: the WASM standard base (1.0, 2.0, 3.0). Default: WASM 3.0
    // Additional_set: additional proposals to turn on. Default: {}
    // Removal_set: additional proposals to turn off. Default: {}
    // Mode: test execution modes (interpreter, AOT, JIT). Default: all
    {"wasm-1.0"sv, WasmEdge::Standard::WASM_1},
    {"wasm-2.0"sv, WasmEdge::Standard::WASM_2},
    {"wasm-3.0"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-bulk-memory"sv, WasmEdge::Standard::WASM_3},
    // TODO: EXCEPTION - implement the AOT.
    {"wasm-3.0-exceptions"sv,
     WasmEdge::Standard::WASM_3,
     {},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"wasm-3.0-gc"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-memory64"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-multi-memory"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-relaxed-simd"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-simd"sv, WasmEdge::Standard::WASM_3},
    {"threads"sv, WasmEdge::Standard::WASM_2, {Proposal::Threads}},
    // Component model only supports interpreter mode currently.
    {"component-model"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
};

// Used for labeling the status of component model support of each test folder.
// Would be deleted when component model is fully supported.
struct ComponentModelSupport {
  bool Load;
  bool Validate;
  bool Instantiate;
  bool Execute;
  bool WatParsing = false;
};

// clang-format off
// Used for labeling the status of component model support of each test folder.
// Would be deleted when component model is fully supported.
std::unordered_map<std::string, ComponentModelSupport, Hash::Hash>
    ComponentModelFolders = {
    // | Folder | Test table: {load, validate, instantiate, execute} |
    // ---------------------------------------------------------------
    // Folder: the directory name of tests.
    // Test table: the testing status of load, validate, instantiate and execute
    {"adapt",                   {true, false, false, false}},
    {"alias",                   {true, false, false, false}},
    {"big",                     {true, false, false, false}},
    {"definedtypes",            {true, true,  true,  false}},
    {"empty",                   {true, true,  true,  false}},
    {"example",                 {true, true,  true,  false}},
    {"export",                  {true, true,  false, false}},
    {"export-ascription",       {true, true,  false, false}},
    {"export-introduces-alias", {true, true,  true,  false}},
    {"func",                    {true, false, false, false}},
    {"import",                  {true, false, false, false}},
    {"imports-exports",         {true, false, false, false}},
    {"inline-exports",          {true, true,  true,  false}},
    {"instance-types",          {true, true,  true,  false}},
    {"instantiate",             {true, false, false, false}},
    {"invalid",                 {true, false, false, false}},
    {"link",                    {true, true,  true,  false}},
    {"lots-of-aliases",         {true, true,  true,  false}},
    {"lower",                   {true, false, false, false}},
    {"memory64",                {true, false, false, false}},
    {"module-link",             {true, false, false, false}},
    {"more-flags",              {true, true,  true,  false}},
    {"naming",                  {true, false, false, false}},
    {"nested-modules",          {true, false, false, false}},
    {"resources",               {true, false, false, false}},
    {"tags",                    {true, false, false, false}},
    {"type-export-restrictions",{true, false, false, false}},
    {"types",                   {true, false, false, false}},
    {"very-nested",             {true, false, false, false}},
    {"virtualize",              {true, false, false, false}},
};
// clang-format on

// Used for getting the status of component model support of each test folder.
// Would be deleted when component model is fully supported.
bool checkComponentSupported(std::string_view Folder, WasmEdge::WasmPhase P) {
  auto It = ComponentModelFolders.find(std::string(Folder));
  if (It == ComponentModelFolders.end()) {
    return false;
  }
  switch (P) {
  case WasmEdge::WasmPhase::Loading:
    return It->second.Load;
  case WasmEdge::WasmPhase::Validation:
    return It->second.Validate;
  case WasmEdge::WasmPhase::Instantiation:
    return It->second.Instantiate;
  case WasmEdge::WasmPhase::Execution:
    return It->second.Execute;
  default:
    return false;
  }
}

} // namespace

namespace WasmEdge {

std::vector<std::string> SpecTest::enumerate(const SpecTest::TestMode Modes,
                                             bool IncludeComponent) const {
  std::vector<std::string> Cases;
  std::string Ext = (this->Mode == ParserMode::Wast) ? ".wast"s : ".json"s;
  for (const auto &Proposal : TestsuiteProposals) {
    if (!(static_cast<uint8_t>(Proposal.Mode) & static_cast<uint8_t>(Modes))) {
      continue;
    }
    if (!IncludeComponent && Proposal.Path == "component-model"sv) {
      continue;
    }
    // WAST mode does not support component-model tests.
    if (this->Mode == ParserMode::Wast &&
        Proposal.Path == "component-model"sv) {
      continue;
    }
    const std::filesystem::path ProposalRoot = TestsuiteRoot / Proposal.Path;
    if (!std::filesystem::exists(ProposalRoot)) {
      continue;
    }
    for (const auto &Subdir :
         std::filesystem::directory_iterator(ProposalRoot)) {
      const auto SubdirPath = Subdir.path();
      const auto UnitName = SubdirPath.filename().u8string();
      const auto UnitFile = UnitName + Ext;
      if (std::filesystem::is_regular_file(SubdirPath / UnitFile)) {
        Cases.push_back(std::string(Proposal.Path) + ' ' + UnitName);
      }
    }
  }
  std::sort(Cases.begin(), Cases.end());
  return Cases;
}

std::tuple<std::string_view, WasmEdge::Configure, std::string>
SpecTest::resolve(std::string_view Params) const {
  const auto Pos = Params.find_last_of(' ');
  const std::string_view ProposalPath = Params.substr(0, Pos);
  const auto &MatchedProposal = *std::find_if(
      std::begin(TestsuiteProposals), std::end(TestsuiteProposals),
      [&ProposalPath](const auto &Proposal) {
        return Proposal.Path == ProposalPath;
      });
  return std::tuple<std::string_view, WasmEdge::Configure, std::string>{
      MatchedProposal.Path, MatchedProposal.Conf, Params.substr(Pos + 1)};
}

bool SpecTest::compareResult(const Wast::Result &Expected,
                             const std::pair<ValVariant, ValType> &Got) const {
  const auto Code = Expected.Type.getCode();
  // For reference types, ValType stores the outer code as Ref/RefNull and the
  // actual heap kind in getHeapTypeCode(). Use the heap type code for dispatch
  // on reference types.
  const auto HTCode = Expected.Type.getHeapTypeCode();

  auto IsRefMatch = [&Expected](const WasmEdge::RefVariant &R) {
    // Opaque reference check: only type matters, any value is fine.
    if (Expected.OpaqueRef) {
      return true;
    }
    // Check if the expected value is null.
    if (Expected.Value.get<RefVariant>().isNull()) {
      return R.isNull();
    }
    // Non-null: compare pointer values (for externref with numeric values).
    auto ExpPtr = reinterpret_cast<uintptr_t>(
        Expected.Value.get<RefVariant>().getPtr<void>());
    auto GotPtr = reinterpret_cast<uintptr_t>(R.getPtr<void>());
    return static_cast<uint32_t>(ExpPtr) == static_cast<uint32_t>(GotPtr);
  };

  // Scalar numeric types: direct bit comparison.
  switch (Code) {
  case TypeCode::I32:
    if (Got.second.getCode() != TypeCode::I32) {
      return false;
    }
    return Got.first.get<uint32_t>() == Expected.Value.get<uint32_t>();
  case TypeCode::F32:
    if (Got.second.getCode() != TypeCode::F32) {
      return false;
    }
    // Handle NaN
    if (Expected.NaN != Wast::Result::NaNPattern::None) {
      if (!std::isnan(Got.first.get<float>())) {
        return false;
      }
      if (Expected.NaN == Wast::Result::NaNPattern::Canonical) {
        uint32_t Bits = Got.first.get<uint32_t>();
        return (Bits & 0x7FFFFFFFU) == 0x7FC00000U;
      }
      return true;
    }
    return Got.first.get<uint32_t>() == Expected.Value.get<uint32_t>();
  case TypeCode::I64:
    if (Got.second.getCode() != TypeCode::I64) {
      return false;
    }
    return Got.first.get<uint64_t>() == Expected.Value.get<uint64_t>();
  case TypeCode::F64:
    if (Got.second.getCode() != TypeCode::F64) {
      return false;
    }
    // Handle NaN
    if (Expected.NaN != Wast::Result::NaNPattern::None) {
      if (!std::isnan(Got.first.get<double>())) {
        return false;
      }
      if (Expected.NaN == Wast::Result::NaNPattern::Canonical) {
        uint64_t Bits = Got.first.get<uint64_t>();
        return (Bits & 0x7FFFFFFFFFFFFFFFULL) == 0x7FF8000000000000ULL;
      }
      return true;
    }
    return Got.first.get<uint64_t>() == Expected.Value.get<uint64_t>();
  default:
    break;
  }

  // Reference type handling. Dispatch on heap type code since getCode()
  // returns Ref/RefNull for all reference types.
  if (Expected.Type.isRefType()) {
    // Handle bare (ref.null) — matches any null reference regardless of type.
    if (Expected.AnyNullRef) {
      if (!Got.second.isRefType()) {
        return false;
      }
      return Got.first.get<RefVariant>().isNull();
    }
    switch (HTCode) {
    case TypeCode::AnyRef:
      // "anyref" fits all internal reference types.
      if (!Got.second.isRefType() || Got.second.isExternRefType()) {
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::EqRef:
      // "eqref" fits eqref, structref, arrayref, i31ref, and nullref.
      if (!Got.second.isRefType()) {
        return false;
      }
      switch (Got.second.getHeapTypeCode()) {
      case TypeCode::EqRef:
      case TypeCode::I31Ref:
      case TypeCode::StructRef:
      case TypeCode::ArrayRef:
      case TypeCode::NullRef:
        break;
      default:
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::StructRef:
      // "structref" fits structref and nullref.
      if (!Got.second.isRefType()) {
        return false;
      }
      switch (Got.second.getHeapTypeCode()) {
      case TypeCode::StructRef:
      case TypeCode::NullRef:
        break;
      default:
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::ArrayRef:
      // "arrayref" fits arrayref and nullref.
      if (!Got.second.isRefType()) {
        return false;
      }
      switch (Got.second.getHeapTypeCode()) {
      case TypeCode::ArrayRef:
      case TypeCode::NullRef:
        break;
      default:
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::I31Ref:
      // "i31ref" fits i31ref and nullref.
      if (!Got.second.isRefType()) {
        return false;
      }
      switch (Got.second.getHeapTypeCode()) {
      case TypeCode::I31Ref:
      case TypeCode::NullRef:
        break;
      default:
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::NullRef:
      // "nullref" (none) is the bottom type for internal refs.
      // It matches any internal reference type (any, eq, i31, struct, array,
      // none).
      if (!Got.second.isRefType() || Got.second.isExternRefType()) {
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::FuncRef:
      // "funcref" fits funcref and nullfuncref.
      if (!Got.second.isFuncRefType()) {
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::NullFuncRef:
      // "nullfuncref" (nofunc) is the bottom type for func refs.
      // It matches funcref and nullfuncref.
      if (!Got.second.isFuncRefType()) {
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::ExternRef:
      // "externref" fits externref and nullexternref.
      if (!Got.second.isExternRefType()) {
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::NullExternRef:
      // "nullexternref" (noextern) is the bottom type for extern refs.
      // It matches externref and nullexternref.
      if (!Got.second.isExternRefType()) {
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::ExnRef:
      // "exnref" fits exnref and nullexnref.
      if (!Got.second.isRefType() ||
          (Got.second.getHeapTypeCode() != TypeCode::ExnRef &&
           Got.second.getHeapTypeCode() != TypeCode::NullExnRef)) {
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::NullExnRef:
      // "nullexnref" (noexn) is the bottom type for exn refs.
      // It matches exnref and nullexnref.
      if (!Got.second.isRefType() ||
          (Got.second.getHeapTypeCode() != TypeCode::ExnRef &&
           Got.second.getHeapTypeCode() != TypeCode::NullExnRef)) {
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    default:
      return false;
    }
  }

  // V128 type.
  if (Code == TypeCode::V128) {
    if (Got.second.getCode() != TypeCode::V128) {
      return false;
    }
    std::string_view Shape = Expected.V128Shape;
    // Determine lane type from shape string (e.g., "f32x4" -> "f32").
    // The shape format is "<type>x<count>".
    auto ShIt = V128ShapeMap().find(Shape);
    auto ShK =
        (ShIt != V128ShapeMap().end()) ? ShIt->second : V128Shape::Unknown;
    LaneKind LK = LaneKind::Unknown;
    switch (ShK) {
    case V128Shape::F32x4:
      LK = LaneKind::F32;
      break;
    case V128Shape::F64x2:
      LK = LaneKind::F64;
      break;
    case V128Shape::I8x16:
      LK = LaneKind::I8;
      break;
    case V128Shape::I16x8:
      LK = LaneKind::I16;
      break;
    case V128Shape::I32x4:
      LK = LaneKind::I32;
      break;
    case V128Shape::I64x2:
      LK = LaneKind::I64;
      break;
    default:
      // Unknown shape or empty — fall back to raw 128-bit comparison.
      return Got.first.get<uint128_t>() == Expected.Value.get<uint128_t>();
    }

    switch (LK) {
    case LaneKind::F32: {
      float GotF[4], ExpF[4];
      uint32_t GotI[4], ExpI[4];
      std::memcpy(GotF, &Got.first.get<uint128_t>(), 16);
      std::memcpy(GotI, &Got.first.get<uint128_t>(), 16);
      std::memcpy(ExpF, &Expected.Value.get<uint128_t>(), 16);
      std::memcpy(ExpI, &Expected.Value.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(GotI, GotI + 4);
        std::reverse(GotF, GotF + 4);
        std::reverse(ExpI, ExpI + 4);
        std::reverse(ExpF, ExpF + 4);
      }
      for (size_t I = 0; I < 4; ++I) {
        if (I < Expected.V128LaneNaN.size() &&
            Expected.V128LaneNaN[I] != Wast::Result::NaNPattern::None) {
          if (!std::isnan(GotF[I])) {
            return false;
          }
          if (Expected.V128LaneNaN[I] == Wast::Result::NaNPattern::Canonical) {
            if ((GotI[I] & 0x7FFFFFFFU) != 0x7FC00000U) {
              return false;
            }
          }
        } else {
          if (GotI[I] != ExpI[I]) {
            return false;
          }
        }
      }
      return true;
    }
    case LaneKind::F64: {
      double GotF[2], ExpF[2];
      uint64_t GotI[2], ExpI[2];
      std::memcpy(GotF, &Got.first.get<uint128_t>(), 16);
      std::memcpy(GotI, &Got.first.get<uint128_t>(), 16);
      std::memcpy(ExpF, &Expected.Value.get<uint128_t>(), 16);
      std::memcpy(ExpI, &Expected.Value.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(GotI, GotI + 2);
        std::reverse(GotF, GotF + 2);
        std::reverse(ExpI, ExpI + 2);
        std::reverse(ExpF, ExpF + 2);
      }
      for (size_t I = 0; I < 2; ++I) {
        if (I < Expected.V128LaneNaN.size() &&
            Expected.V128LaneNaN[I] != Wast::Result::NaNPattern::None) {
          if (!std::isnan(GotF[I])) {
            return false;
          }
          if (Expected.V128LaneNaN[I] == Wast::Result::NaNPattern::Canonical) {
            if ((GotI[I] & 0x7FFFFFFFFFFFFFFFULL) != 0x7FF8000000000000ULL) {
              return false;
            }
          }
        } else {
          if (GotI[I] != ExpI[I]) {
            return false;
          }
        }
      }
      return true;
    }
    case LaneKind::I8: {
      uint8_t GotV[16], ExpV[16];
      std::memcpy(GotV, &Got.first.get<uint128_t>(), 16);
      std::memcpy(ExpV, &Expected.Value.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(GotV, GotV + 16);
        std::reverse(ExpV, ExpV + 16);
      }
      for (size_t I = 0; I < 16; ++I) {
        if (GotV[I] != ExpV[I]) {
          return false;
        }
      }
      return true;
    }
    case LaneKind::I16: {
      uint16_t GotV[8], ExpV[8];
      std::memcpy(GotV, &Got.first.get<uint128_t>(), 16);
      std::memcpy(ExpV, &Expected.Value.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(GotV, GotV + 8);
        std::reverse(ExpV, ExpV + 8);
      }
      for (size_t I = 0; I < 8; ++I) {
        if (GotV[I] != ExpV[I]) {
          return false;
        }
      }
      return true;
    }
    case LaneKind::I32: {
      uint32_t GotV[4], ExpV[4];
      std::memcpy(GotV, &Got.first.get<uint128_t>(), 16);
      std::memcpy(ExpV, &Expected.Value.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(GotV, GotV + 4);
        std::reverse(ExpV, ExpV + 4);
      }
      for (size_t I = 0; I < 4; ++I) {
        if (GotV[I] != ExpV[I]) {
          return false;
        }
      }
      return true;
    }
    case LaneKind::I64: {
      uint64_t GotV[2], ExpV[2];
      std::memcpy(GotV, &Got.first.get<uint128_t>(), 16);
      std::memcpy(ExpV, &Expected.Value.get<uint128_t>(), 16);
      if constexpr (Endian::native == Endian::big) {
        std::reverse(GotV, GotV + 2);
        std::reverse(ExpV, ExpV + 2);
      }
      for (size_t I = 0; I < 2; ++I) {
        if (GotV[I] != ExpV[I]) {
          return false;
        }
      }
      return true;
    }
    default:
      break;
    }
  }

  return false;
}

bool SpecTest::compareResults(
    const std::vector<Wast::ResultOrEither> &Expected,
    const std::vector<std::pair<ValVariant, ValType>> &Got) const {
  if (Expected.size() != Got.size()) {
    return false;
  }
  for (size_t I = 0; I < Expected.size(); ++I) {
    bool Matched = false;
    for (const auto &Alt : Expected[I].Alternatives) {
      if (compareResult(Alt, Got[I])) {
        Matched = true;
        break;
      }
    }
    if (!Matched) {
      return false;
    }
  }
  return true;
}

bool SpecTest::stringContains(std::string_view Expected,
                              std::string_view Got) const {
  if (Expected.rfind(Got, 0) != 0) {
    spdlog::error("   ##### expected text : {}"sv, Expected);
    spdlog::error("   ######## error text : {}"sv, Got);
    return false;
  }
  return true;
}

void SpecTest::executeCommands(Span<const Wast::ScriptCommand> Commands,
                               const Configure &Conf, bool IsComponent,
                               ContextHandle RootCtx, std::string TestFile) {
  std::unordered_map<std::string, std::string, Hash::Hash> Alias;
  std::unordered_map<std::string, SpecTest::WasmUnit, Hash::Hash> ASTMap;
  std::string LastModName;
  uint32_t LastModLine = 0;
  std::unordered_map<std::string, std::thread, Hash::Hash> ThreadMap;

  // Stable storage for resolved file paths (string_view lifetime).
  std::deque<std::string> ResolvedPaths;

  // For component-model tests, extract the unit name (last path component)
  // for use with checkComponentSupported().
  std::string UnitName;
  if (IsComponent) {
    std::filesystem::path PF{TestFile};
    UnitName = PF.filename().u8string();
  }

  // Helper: resolve source for onParse. Returns (source_view, ModuleType).
  // For file-based modules, resolves the full path and stores it for lifetime.
  auto resolveSource = [&](const Wast::ScriptCommand &Cmd)
      -> std::pair<std::string_view, Wast::ModuleType> {
    if (Cmd.ModType == Wast::ModuleType::TextFile ||
        Cmd.ModType == Wast::ModuleType::BinaryFile) {
      auto Path = std::filesystem::u8path(TestFile).parent_path() /
                  std::string(Cmd.ModuleSource);
      ResolvedPaths.push_back(Path.string());
      return {ResolvedPaths.back(), Cmd.ModType};
    }
    return {Cmd.ModuleSource, Cmd.ModType};
  };

  // Preprocessing: resolve register aliases.
  for (const auto &Cmd : Commands) {
    if (Cmd.Type == Wast::CommandType::Module ||
        Cmd.Type == Wast::CommandType::ModuleDefinition ||
        Cmd.Type == Wast::CommandType::ModuleInstance) {
      if (Cmd.ModuleName) {
        LastModName = std::string(*Cmd.ModuleName);
      } else {
        LastModName.clear();
      }
      LastModLine = Cmd.Line;
    } else if (Cmd.Type == Wast::CommandType::Register) {
      if (Cmd.ModuleName) {
        Alias.emplace(std::string(*Cmd.ModuleName),
                      std::string(Cmd.RegisterName));
      } else if (!LastModName.empty()) {
        Alias.emplace(LastModName, std::string(Cmd.RegisterName));
      } else {
        Alias.emplace(std::to_string(LastModLine),
                      std::string(Cmd.RegisterName));
      }
    }
  }

  // Reset tracking state for command execution.
  LastModName.clear();
  LastModLine = 0;

  // Helper to resolve module name for actions.
  auto GetModuleName = [&](const Wast::Action &Act) -> std::string {
    if (Act.ModuleName) {
      std::string MN(Act.ModuleName.value());
      if (auto It = Alias.find(MN); It != Alias.end()) {
        return It->second;
      }
      return MN;
    }
    return LastModName;
  };

  // Execute commands.
  for (const auto &Cmd : Commands) {
    switch (Cmd.Type) {
    case Wast::CommandType::Module: {
      std::string ModName;
      if (Cmd.ModuleName) {
        ModName = std::string(*Cmd.ModuleName);
        if (auto It = Alias.find(ModName); It != Alias.end()) {
          ModName = It->second;
        }
      } else {
        std::string LineStr = std::to_string(Cmd.Line);
        if (auto It = Alias.find(LineStr); It != Alias.end()) {
          ModName = It->second;
        }
      }
      LastModName = ModName;
      LastModLine = Cmd.Line;

      // Reset per-command; only set for components missing validation support.
      SkipComponentValidation = false;

      if (IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Instantiation)) {
        // Instantiation not supported: fall back to validate or load.
        auto [Source, Type] = resolveSource(Cmd);
        auto ParseRes = onParse(RootCtx, Source, Type, Conf);
        if (!ParseRes) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "Module parse failed: "
              << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
          break;
        }
        if (checkComponentSupported(UnitName, WasmPhase::Validation)) {
          auto ValRes = onValidate(RootCtx, *ParseRes);
          if (!ValRes) {
            ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
                << "Module validate failed: "
                << WasmEdge::ErrCodeStr[ValRes.error().getEnum()];
          }
        }
        break;
      }
      SkipComponentValidation =
          IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Validation);

      auto [Source, Type] = resolveSource(Cmd);

      // onParse
      auto ParseRes = onParse(RootCtx, Source, Type, Conf);
      if (!ParseRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Module parse failed: "
            << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
        break;
      }

      // onValidate (skip if component validation unsupported)
      if (!SkipComponentValidation) {
        auto ValRes = onValidate(RootCtx, *ParseRes);
        if (!ValRes) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "Module validate failed: "
              << WasmEdge::ErrCodeStr[ValRes.error().getEnum()];
          break;
        }
      }

      // onInstantiate
      auto InstRes = onInstantiate(RootCtx, ModName, *ParseRes);
      if (!InstRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Module instantiate failed: "
            << WasmEdge::ErrCodeStr[InstRes.error().getEnum()];
      }
      break;
    }
    case Wast::CommandType::ModuleDefinition: {
      std::string ModName;
      if (Cmd.ModuleName) {
        ModName = std::string(*Cmd.ModuleName);
        if (auto It = Alias.find(ModName); It != Alias.end()) {
          ModName = It->second;
        }
      } else {
        std::string LineStr = std::to_string(Cmd.Line);
        if (auto It = Alias.find(LineStr); It != Alias.end()) {
          ModName = It->second;
        }
      }
      LastModName = ModName;
      LastModLine = Cmd.Line;

      // Reset per-command; only set for components missing validation support.
      SkipComponentValidation = false;

      if (IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Loading)) {
        break;
      }
      SkipComponentValidation =
          IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Validation);

      auto [Source, Type] = resolveSource(Cmd);

      // onParse
      auto ParseRes = onParse(RootCtx, Source, Type, Conf);
      if (!ParseRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Module define parse failed: "
            << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
        break;
      }

      // onValidate
      if (!SkipComponentValidation) {
        auto ValRes = onValidate(RootCtx, *ParseRes);
        if (!ValRes) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "Module define validate failed: "
              << WasmEdge::ErrCodeStr[ValRes.error().getEnum()];
          break;
        }
      }

      // Store in ASTMap for later instantiation.
      if (Cmd.ModuleName) {
        ASTMap.emplace(std::string(*Cmd.ModuleName), std::move(*ParseRes));
      }
      break;
    }
    case Wast::CommandType::ModuleInstance: {
      if (IsComponent) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Unexpected module_instance for component";
        break;
      }
      std::string ModName;
      if (Cmd.ModuleName) {
        ModName = std::string(*Cmd.ModuleName);
        if (auto It = Alias.find(ModName); It != Alias.end()) {
          ModName = It->second;
        }
      }
      LastModName = ModName;
      LastModLine = Cmd.Line;

      if (Cmd.DefinitionName) {
        auto ASTDef = ASTMap.find(std::string(*Cmd.DefinitionName));
        if (ASTDef != ASTMap.end()) {
          if (auto Res = onInstantiate(RootCtx, ModName, ASTDef->second)) {
            // success
          } else {
            ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
                << "Module instance failed: "
                << WasmEdge::ErrCodeStr[Res.error().getEnum()];
          }
        } else {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "Module definition not found: " << *Cmd.DefinitionName;
        }
      } else {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Module instance missing definition name";
      }
      break;
    }
    case Wast::CommandType::Register: {
      // Already handled in preprocessing.
      break;
    }
    case Wast::CommandType::Action: {
      if (IsComponent) {
        break;
      }
      if (!Cmd.Act) {
        break;
      }
      const auto &Act = *Cmd.Act;
      const auto ModName = GetModuleName(Act);
      if (Act.Type == Wast::ActionType::Invoke) {
        if (auto Res = onInvoke(RootCtx, ModName, std::string(Act.FieldName),
                                Act.Args, Act.ArgTypes)) {
          // success
        } else {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line) << "Action invoke failed";
        }
      } else {
        if (auto Res = onGet(RootCtx, ModName, std::string(Act.FieldName))) {
          // success
        } else {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line) << "Action get failed";
        }
      }
      break;
    }
    case Wast::CommandType::AssertReturn: {
      if (IsComponent) {
        break;
      }
      if (!Cmd.Act) {
        break;
      }
      const auto &Act = *Cmd.Act;
      const auto ModName = GetModuleName(Act);

      if (Act.Type == Wast::ActionType::Invoke) {
        auto Res = onInvoke(RootCtx, ModName, std::string(Act.FieldName),
                            Act.Args, Act.ArgTypes);
        if (!Res) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "assert_return invoke failed: "
              << WasmEdge::ErrCodeStr[Res.error().getEnum()];
          break;
        }
        EXPECT_TRUE(compareResults(Cmd.Expected, *Res))
            << TestFile << ":" << Cmd.Line << ": assert_return mismatch";
      } else {
        auto Res = onGet(RootCtx, ModName, std::string(Act.FieldName));
        if (!Res) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "assert_return get failed";
          break;
        }
        if (!Cmd.Expected.empty()) {
          std::vector<std::pair<ValVariant, ValType>> GotVec = {*Res};
          EXPECT_TRUE(compareResults(Cmd.Expected, GotVec))
              << TestFile << ":" << Cmd.Line << ": assert_return get mismatch";
        }
      }
      break;
    }
    case Wast::CommandType::AssertTrap: {
      if (Cmd.Act) {
        if (IsComponent &&
            !checkComponentSupported(UnitName, WasmPhase::Execution)) {
          break;
        }
        const auto &Act = *Cmd.Act;
        const auto ModName = GetModuleName(Act);
        auto Res = onInvoke(RootCtx, ModName, std::string(Act.FieldName),
                            Act.Args, Act.ArgTypes);
        if (Res) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "assert_trap expected trap";
        } else {
          EXPECT_TRUE(Res.error().getErrCodePhase() ==
                      WasmEdge::WasmPhase::Execution)
              << TestFile << ":" << Cmd.Line << ": assert_trap wrong phase";
          EXPECT_TRUE(
              stringContains(std::string(Cmd.ExpectedMessage),
                             WasmEdge::ErrCodeStr[Res.error().getEnum()]))
              << TestFile << ":" << Cmd.Line
              << ": assert_trap message mismatch";
        }
      } else if (!Cmd.ModuleSource.empty()) {
        // Trap on module instantiation: onParse -> onValidate -> onInstantiate
        auto [Source, Type] = resolveSource(Cmd);

        auto ParseRes = onParse(RootCtx, Source, Type, Conf);
        if (!ParseRes) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "assert_trap parse unexpectedly failed: "
              << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
          break;
        }
        auto ValRes = onValidate(RootCtx, *ParseRes);
        if (!ValRes) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "assert_trap validate unexpectedly failed: "
              << WasmEdge::ErrCodeStr[ValRes.error().getEnum()];
          break;
        }
        auto InstRes = onInstantiate(RootCtx, "", *ParseRes);
        if (InstRes) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "assert_trap expected trap";
        } else {
          EXPECT_TRUE(InstRes.error().getErrCodePhase() ==
                          WasmEdge::WasmPhase::Instantiation ||
                      InstRes.error().getErrCodePhase() ==
                          WasmEdge::WasmPhase::Execution)
              << TestFile << ":" << Cmd.Line << ": assert_trap wrong phase";
        }
      }
      break;
    }
    case Wast::CommandType::AssertExhaustion: {
      // TODO: Add stack overflow mechanism.
      break;
    }
    case Wast::CommandType::AssertInvalid: {
      if (IsComponent && Cmd.ModType != Wast::ModuleType::BinaryFile &&
          Cmd.ModType != Wast::ModuleType::Binary) {
        break;
      }
      if (IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Validation)) {
        break;
      }

      auto [Source, Type] = resolveSource(Cmd);

      // onParse
      auto ParseRes = onParse(RootCtx, Source, Type, Conf);
      if (!ParseRes) {
        // WAT parser may catch validation errors during loading.
        EXPECT_TRUE(ParseRes.error().getErrCodePhase() ==
                        WasmEdge::WasmPhase::Validation ||
                    ParseRes.error().getErrCodePhase() ==
                        WasmEdge::WasmPhase::Loading)
            << TestFile << ":" << Cmd.Line << ": assert_invalid wrong phase: "
            << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
        if (!Cmd.ExpectedMessage.empty()) {
          EXPECT_TRUE(
              stringContains(std::string(Cmd.ExpectedMessage),
                             WasmEdge::ErrCodeStr[ParseRes.error().getEnum()]))
              << TestFile << ":" << Cmd.Line
              << ": assert_invalid message mismatch";
        }
        break;
      }

      // onValidate - expect failure here
      auto ValRes = onValidate(RootCtx, *ParseRes);
      if (ValRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_invalid expected failure";
      } else {
        EXPECT_TRUE(ValRes.error().getErrCodePhase() ==
                        WasmEdge::WasmPhase::Validation ||
                    ValRes.error().getErrCodePhase() ==
                        WasmEdge::WasmPhase::Loading)
            << TestFile << ":" << Cmd.Line << ": assert_invalid wrong phase: "
            << WasmEdge::ErrCodeStr[ValRes.error().getEnum()];
        if (!Cmd.ExpectedMessage.empty()) {
          EXPECT_TRUE(
              stringContains(std::string(Cmd.ExpectedMessage),
                             WasmEdge::ErrCodeStr[ValRes.error().getEnum()]))
              << TestFile << ":" << Cmd.Line
              << ": assert_invalid message mismatch";
        }
      }
      break;
    }
    case Wast::CommandType::AssertMalformed: {
      if (IsComponent && Cmd.ModType != Wast::ModuleType::BinaryFile &&
          Cmd.ModType != Wast::ModuleType::Binary) {
        break;
      }
      if (IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Loading)) {
        break;
      }

      auto [Source, Type] = resolveSource(Cmd);

      // onParse only - expect failure
      auto ParseRes = onParse(RootCtx, Source, Type, Conf);
      if (ParseRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_malformed expected failure";
      } else {
        // Accept loading or validation phase errors (WAT parser may report
        // validation errors during parsing).
        EXPECT_TRUE(ParseRes.error().getErrCodePhase() ==
                        WasmEdge::WasmPhase::Loading ||
                    ParseRes.error().getErrCodePhase() ==
                        WasmEdge::WasmPhase::Validation)
            << TestFile << ":" << Cmd.Line << ": assert_malformed wrong phase: "
            << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
        if (!Cmd.ExpectedMessage.empty()) {
          EXPECT_TRUE(
              stringContains(std::string(Cmd.ExpectedMessage),
                             WasmEdge::ErrCodeStr[ParseRes.error().getEnum()]))
              << TestFile << ":" << Cmd.Line
              << ": assert_malformed message mismatch";
        }
      }
      break;
    }
    case Wast::CommandType::AssertUnlinkable:
    case Wast::CommandType::AssertUninstantiable: {
      if (IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Instantiation)) {
        break;
      }

      auto [Source, Type] = resolveSource(Cmd);

      // onParse -> onValidate -> onInstantiate, expect error at instantiate.
      auto ParseRes = onParse(RootCtx, Source, Type, Conf);
      if (!ParseRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_unlinkable/uninstantiable parse unexpectedly failed: "
            << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
        break;
      }
      auto ValRes = onValidate(RootCtx, *ParseRes);
      if (!ValRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_unlinkable/uninstantiable validate unexpectedly failed: "
            << WasmEdge::ErrCodeStr[ValRes.error().getEnum()];
        break;
      }
      auto InstRes = onInstantiate(RootCtx, "", *ParseRes);
      if (InstRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_unlinkable/uninstantiable expected failure";
      } else {
        EXPECT_TRUE(InstRes.error().getErrCodePhase() ==
                        WasmEdge::WasmPhase::Instantiation ||
                    InstRes.error().getErrCodePhase() ==
                        WasmEdge::WasmPhase::Execution)
            << TestFile << ":" << Cmd.Line
            << ": assert_unlinkable/uninstantiable wrong phase";
        EXPECT_TRUE(
            stringContains(std::string(Cmd.ExpectedMessage),
                           WasmEdge::ErrCodeStr[InstRes.error().getEnum()]))
            << TestFile << ":" << Cmd.Line
            << ": assert_unlinkable/uninstantiable message mismatch";
      }
      break;
    }
    case Wast::CommandType::AssertException: {
      if (IsComponent) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_exception not supported for components";
        break;
      }
      if (Cmd.Act) {
        const auto &Act = *Cmd.Act;
        const auto ModName = GetModuleName(Act);
        auto Res = onInvoke(RootCtx, ModName, std::string(Act.FieldName),
                            Act.Args, Act.ArgTypes);
        if (Res) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "assert_exception expected exception";
        } else {
          EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::UncaughtException)
              << TestFile << ":" << Cmd.Line
              << ": assert_exception wrong error";
        }
      }
      break;
    }
    case Wast::CommandType::Thread: {
      if (!onInit) {
        break;
      }
      std::string ThreadName;
      if (Cmd.ModuleName) {
        ThreadName = std::string(*Cmd.ModuleName);
      }

      if (ThreadMap.find(ThreadName) != ThreadMap.end()) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Duplicate thread name: " << ThreadName;
        break;
      }

      // Pre-scan thread sub-commands for register entries to determine
      // alias names for shared modules.
      std::unordered_map<std::string, std::string, Hash::Hash>
          SharedRegisterMap;
      for (const auto &SubCmd : Cmd.SubCommands) {
        if (SubCmd.Type == Wast::CommandType::Register && SubCmd.ModuleName) {
          SharedRegisterMap.emplace(std::string(*SubCmd.ModuleName),
                                    std::string(SubCmd.RegisterName));
        }
      }

      std::vector<std::pair<std::string, std::string>> SharedModules;
      for (const auto &[OrigName, AliasName] : Cmd.SharedModules) {
        std::string ParentName = OrigName;
        if (auto It = Alias.find(OrigName); It != Alias.end()) {
          ParentName = It->second;
        }
        std::string FinalAlias = AliasName;
        if (FinalAlias.empty()) {
          if (auto It = SharedRegisterMap.find(OrigName);
              It != SharedRegisterMap.end()) {
            FinalAlias = It->second;
          } else {
            FinalAlias = ParentName;
          }
        }
        SharedModules.emplace_back(std::move(ParentName),
                                   std::move(FinalAlias));
      }

      auto ChildCtx = onInit(RootCtx, SharedModules);

      ThreadMap.emplace(
          ThreadName, std::thread([this, ChildCtx, SubCmds = Cmd.SubCommands,
                                   Conf, IsComponent, TestFile]() mutable {
            executeCommands(SubCmds, Conf, IsComponent, ChildCtx, TestFile);
            onFini(ChildCtx);
          }));
      break;
    }
    case Wast::CommandType::Wait: {
      if (Cmd.ThreadName) {
        auto It = ThreadMap.find(std::string(*Cmd.ThreadName));
        if (It != ThreadMap.end()) {
          if (It->second.joinable()) {
            It->second.join();
          }
          ThreadMap.erase(It);
        } else {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "Wait for unknown thread: " << *Cmd.ThreadName;
        }
      }
      break;
    }
    }
  }
  // Safety: join any threads not explicitly waited on.
  for (auto &[Name, Thread] : ThreadMap) {
    if (Thread.joinable()) {
      Thread.join();
    }
  }
}

void SpecTest::run(std::string_view Proposal, std::string_view UnitName) {
  const auto [ProposalPath, Conf, UName] =
      resolve(std::string(Proposal) + " " + std::string(UnitName));
  auto RootCtx = onInit(nullptr, {});
  bool IsComp = ProposalPath.find("component-model") != std::string_view::npos;

  if (Mode == ParserMode::Json) {
    auto JsonPath =
        TestsuiteRoot / ProposalPath / UName / (std::string(UName) + ".json");
    auto ScriptResult = Wast::parseJson(JsonPath);
    if (!ScriptResult) {
      onFini(RootCtx);
      GTEST_SKIP() << "Failed to parse JSON: " << JsonPath;
    }
    executeCommands(ScriptResult->Commands, Conf, IsComp, RootCtx,
                    JsonPath.u8string());
  } else {
    spdlog::info("{} {}"sv, Proposal, UnitName);
    auto WastPath =
        TestsuiteRoot / ProposalPath / UName / (std::string(UName) + ".wast"s);
    auto ScriptResult = Wast::parseWast(WastPath);
    if (!ScriptResult) {
      spdlog::error("Failed to parse WAST file: {}"sv, WastPath.u8string());
      ADD_FAILURE_AT(WastPath.u8string().c_str(), 1)
          << "Failed to parse WAST: " << WastPath.u8string();
      onFini(RootCtx);
      return;
    }
    executeCommands(ScriptResult->Commands, Conf, IsComp, RootCtx,
                    WastPath.u8string());
  }
  onFini(RootCtx);
}

} // namespace WasmEdge

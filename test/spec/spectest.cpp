// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/spec/spectest.cpp - Wasm test suites ----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file parses and runs the Wasm test suites. The suites come from the
/// JSON that wast2json extracts, or directly from the .wast scripts through
/// the WAST parser of tree-sitter.
/// Test Suites: https://github.com/WebAssembly/testsuite
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "spectest.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/hash.h"
#include "common/spdlog.h"
#include "json_parser.h"
#include "wast_parser.h"

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <variant>

namespace WasmEdge {
thread_local bool SpecTest::SkipComponentValidation = false;
}

namespace {

using namespace std::literals;
using namespace WasmEdge;

// Compare the float lanes of two V128 values. A lane with a NaN pattern
// matches any NaN of that pattern, and the other lanes match bit for bit.
template <typename FloatT, typename BitsT>
bool compareFloatLanes(const uint128_t &GotV, const uint128_t &ExpV,
                       const std::vector<Wast::Result::NaNPattern> &LaneNaN,
                       BitsT CanonicalBits) {
  constexpr size_t LaneCount = sizeof(uint128_t) / sizeof(BitsT);
  constexpr BitsT SignMask = static_cast<BitsT>(~BitsT{0} >> 1);
  BitsT GotBits[LaneCount], ExpBits[LaneCount];
  std::memcpy(GotBits, &GotV, sizeof(uint128_t));
  std::memcpy(ExpBits, &ExpV, sizeof(uint128_t));
  if constexpr (Endian::native == Endian::big) {
    std::reverse(GotBits, GotBits + LaneCount);
    std::reverse(ExpBits, ExpBits + LaneCount);
  }
  for (size_t I = 0; I < LaneCount; ++I) {
    const auto NaN =
        I < LaneNaN.size() ? LaneNaN[I] : Wast::Result::NaNPattern::None;
    if (NaN == Wast::Result::NaNPattern::None) {
      if (GotBits[I] != ExpBits[I]) {
        return false;
      }
      continue;
    }
    FloatT GotF;
    std::memcpy(&GotF, &GotBits[I], sizeof(FloatT));
    if (!std::isnan(GotF)) {
      return false;
    }
    if (NaN == Wast::Result::NaNPattern::Canonical &&
        (GotBits[I] & SignMask) != CanonicalBits) {
      return false;
    }
  }
  return true;
}

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
    // Every runner loads the text modules of the suite through the loader,
    // so the experimental text format input is on for every folder.
    Conf.setEnableWAT(true);
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
    {"wasm-3.0-exceptions"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-gc"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-memory64"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-multi-memory"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-relaxed-simd"sv, WasmEdge::Standard::WASM_3},
    {"wasm-3.0-simd"sv, WasmEdge::Standard::WASM_3},
    {"threads"sv, WasmEdge::Standard::WASM_2, {Proposal::Threads}},
    // Currently, the component model supports only interpreter mode.
    {"component-model-wasm-tools"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
};

// This structure gives the component model support of each test folder.
// Delete this structure after the component model is complete.
struct ComponentModelSupport {
  bool Load;
  bool Validate;
  bool Instantiate;
  bool Execute;
  bool WatParsing = false;
};

// clang-format off
// This map gives the component model support of each test folder.
// Delete this map after the component model is complete.
std::unordered_map<std::string, ComponentModelSupport, Hash::Hash>
    ComponentModelFolders = {
    // | Folder | Test table: {load, validate, instantiate, execute} |
    // ---------------------------------------------------------------
    // Folder: the directory name of tests.
    // Test table: the testing status of load, validate, instantiate, and execute.
    {"adapt",                    {true, true, false, false}},
    {"alias",                    {true, true, false, false}},
    {"big",                      {true, true, false, false}},
    {"definedtypes",             {true, true, true, false}},
    {"empty",                    {true, true, true, false}},
    {"example",                  {true, true, true, false}},
    {"export",                   {true, true, false, false}},
    {"export-ascription",        {true, true, false, false}},
    {"export-introduces-alias",  {true, true, true, false}},
    {"func",                     {true, true, true, true}},
    {"import",                   {true, true, false, false}},
    {"imports-exports",          {true, true, false, false}},
    {"inline-exports",           {true, true, true, false}},
    {"instance-type",            {true, true, true, false}},
    {"instantiate",              {true, true, false, false}},
    {"invalid",                  {true, true, false, false}},
    {"link",                     {true, true, true, false}},
    {"lots-of-aliases",          {true, true, true, false}},
    {"lower",                    {true, true, false, false}},
    {"memory64",                 {true, true, false, false}},
    {"module-link",              {true, true, false, false}},
    {"more-flags",               {true, true, true, false}},
    {"naming",                   {true, true, false, false}},
    {"nested-modules",           {true, true, false, false}},
    {"resources",                {true, true, false, false}},
    {"tags",                     {true, true, true, true}},
    {"type-export-restrictions", {true, true, false, false}},
    {"types",                    {true, true, false, false}},
    {"very-nested",              {true, true, false, false}},
    {"virtualize",               {true, true, false, false}},
};
// clang-format on

// Get the component model support of one test folder.
// Delete this function after the component model is complete.
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
    if (!IncludeComponent && Proposal.Path == "component-model-wasm-tools"sv) {
      continue;
    }
    // The WAST mode does not support the component-model tests.
    if (this->Mode == ParserMode::Wast &&
        Proposal.Path == "component-model-wasm-tools"sv) {
      continue;
    }
    const std::filesystem::path ProposalRoot = TestsuiteRoot / Proposal.Path;
    if (!std::filesystem::exists(ProposalRoot)) {
      continue;
    }
    for (const auto &Subdir :
         std::filesystem::directory_iterator(ProposalRoot)) {
      const auto SubdirPath = Subdir.path();
      const auto UnitName = u8string(SubdirPath.filename());
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
  // For a reference type, getCode() gives only Ref or RefNull. The heap kind
  // comes from getHeapTypeCode(), so select the code from that value.
  const auto HTCode = Expected.Type.getHeapTypeCode();

  auto IsRefMatch = [&Expected](const WasmEdge::RefVariant &R) {
    // This is an opaque reference. Only the type is important, and every value
    // is correct.
    if (Expected.OpaqueRef) {
      return true;
    }
    // Find if the expected value is null.
    if (Expected.Value.get<RefVariant>().isNull()) {
      return R.isNull();
    }
    // The value is not null. Compare the pointer values. An externref can hold
    // a numeric value.
    auto ExpPtr = reinterpret_cast<uintptr_t>(
        Expected.Value.get<RefVariant>().getPtr<void>());
    auto GotPtr = reinterpret_cast<uintptr_t>(R.getPtr<void>());
    return static_cast<uint32_t>(ExpPtr) == static_cast<uint32_t>(GotPtr);
  };

  // These are the scalar numeric types. Compare their bits directly.
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
    // Compare a NaN value.
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
    // Compare a NaN value.
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

  // These are the reference types. Select the code from the heap type code, as
  // the comment at the start of this function says.
  if (Expected.Type.isRefType()) {
    // Compare a bare (ref.null). It matches every null reference, and the type
    // is not important.
    if (Expected.AnyNullRef) {
      if (!Got.second.isRefType()) {
        return false;
      }
      return Got.first.get<RefVariant>().isNull();
    }
    switch (HTCode) {
    case TypeCode::AnyRef:
    case TypeCode::NullRef:
      // "anyref" fits all internal reference types. The "nullref" type, which
      // is none, is the bottom type of the internal references, and it also
      // matches every internal reference.
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
    case TypeCode::FuncRef:
    case TypeCode::NullFuncRef:
      // "funcref" fits funcref and nullfuncref. The "nullfuncref" type, which
      // is nofunc, is the bottom type of the func references.
      if (!Got.second.isFuncRefType()) {
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::ExternRef:
    case TypeCode::NullExternRef:
      // "externref" fits externref and nullexternref. The "nullexternref" type,
      // which is noextern, is the bottom type of the extern references.
      if (!Got.second.isExternRefType()) {
        return false;
      }
      return IsRefMatch(Got.first.get<RefVariant>());
    case TypeCode::ExnRef:
    case TypeCode::NullExnRef:
      // "exnref" fits exnref and nullexnref. The "nullexnref" type, which is
      // noexn, is the bottom type of the exn references.
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

  // This is the V128 type.
  if (Code == TypeCode::V128) {
    if (Got.second.getCode() != TypeCode::V128) {
      return false;
    }
    const auto &GotV = Got.first.get<uint128_t>();
    const auto &ExpV = Expected.Value.get<uint128_t>();
    // Only the float shapes need a compare lane by lane, for the NaN patterns.
    // For every other shape, all lanes match if all 128 bits match.
    if (Expected.V128Shape == "f32x4"sv) {
      return compareFloatLanes<float, uint32_t>(
          GotV, ExpV, Expected.V128LaneNaN, UINT32_C(0x7FC00000));
    }
    if (Expected.V128Shape == "f64x2"sv) {
      return compareFloatLanes<double, uint64_t>(
          GotV, ExpV, Expected.V128LaneNaN, UINT64_C(0x7FF8000000000000));
    }
    return GotV == ExpV;
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

  // On Windows, the assertion macros of googletest have a race when a new
  // thread calls them. Each thread sends its assertions to a thread-local
  // reporter, and the reporter appends them here under the mutex. The parent
  // thread reports them again after the join.
  std::mutex ThreadResultMutex;
  std::vector<testing::TestPartResult> ThreadResults;

  // This is the stable storage of the resolved file paths. It keeps each
  // string_view valid.
  std::deque<std::string> ResolvedPaths;

  // This is the unit name for checkComponentSupported(). Use stem(), and do
  // not use filename(). A key of ComponentModelFolders is a bare folder name
  // with no extension. For example, ".../empty/empty.json" gives "empty".
  std::string UnitName;
  if (IsComponent) {
    std::filesystem::path PF{TestFile};
    UnitName = u8string(PF.stem());
  }

  // Resolve the source for onParse. For a module that comes from a file, this
  // lambda resolves the full path and stores it in ResolvedPaths. The stored
  // path then holds the bytes of the view that the lambda returns.
  auto resolveSource = [&](const Wast::ScriptCommand &Cmd)
      -> std::pair<std::string_view, Wast::ModuleType> {
    if (Cmd.ModType == Wast::ModuleType::TextFile ||
        Cmd.ModType == Wast::ModuleType::BinaryFile) {
      auto Path =
          u8path(TestFile).parent_path() / std::string(Cmd.ModuleSource);
      ResolvedPaths.push_back(Path.string());
      return {ResolvedPaths.back(), Cmd.ModType};
    }
    return {Cmd.ModuleSource, Cmd.ModType};
  };

  // This is the preprocessing step. It resolves the register aliases.
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

  // Clear the state that the command run keeps.
  LastModName.clear();

  // This lambda resolves the name of a module command through the register
  // aliases. A module without a name has the alias of its line number. The
  // name becomes the target of the actions that do not name a module.
  auto enterModule = [&](const Wast::ScriptCommand &Cmd) -> std::string {
    std::string ModName = Cmd.ModuleName ? std::string(*Cmd.ModuleName) : ""s;
    const auto Key = Cmd.ModuleName ? ModName : std::to_string(Cmd.Line);
    if (auto It = Alias.find(Key); It != Alias.end()) {
      ModName = It->second;
    }
    LastModName = ModName;
    return ModName;
  };

  // This lambda resolves the module name of an action.
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

  // Run the commands.
  for (const auto &Cmd : Commands) {
    switch (Cmd.Type) {
    case Wast::CommandType::Module: {
      const auto ModName = enterModule(Cmd);

      // Clear this flag for each command. Only a component with no
      // validation support sets it.
      SkipComponentValidation = false;

      if (IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Instantiation)) {
        // The instantiation has no support. Do the validate step or the load
        // step.
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

      // Run onParse.
      auto ParseRes = onParse(RootCtx, Source, Type, Conf);
      if (!ParseRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Module parse failed: "
            << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
        break;
      }

      // Run onValidate. Skip it if the component validation has no support.
      if (!SkipComponentValidation) {
        auto ValRes = onValidate(RootCtx, *ParseRes);
        if (!ValRes) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "Module validate failed: "
              << WasmEdge::ErrCodeStr[ValRes.error().getEnum()];
          break;
        }
      }

      // Run onInstantiate.
      auto InstRes = onInstantiate(RootCtx, ModName, *ParseRes);
      if (!InstRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Module instantiate failed: "
            << WasmEdge::ErrCodeStr[InstRes.error().getEnum()];
      }
      break;
    }
    case Wast::CommandType::ModuleDefinition: {
      const auto ModName = enterModule(Cmd);

      // Clear this flag for each command. Only a component with no
      // validation support sets it.
      SkipComponentValidation = false;

      if (IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Loading)) {
        break;
      }
      SkipComponentValidation =
          IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Validation);

      auto [Source, Type] = resolveSource(Cmd);

      // Run onParse.
      auto ParseRes = onParse(RootCtx, Source, Type, Conf);
      if (!ParseRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Module define parse failed: "
            << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
        break;
      }

      // Run onValidate.
      if (!SkipComponentValidation) {
        auto ValRes = onValidate(RootCtx, *ParseRes);
        if (!ValRes) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "Module define validate failed: "
              << WasmEdge::ErrCodeStr[ValRes.error().getEnum()];
          break;
        }
      }

      // Store the unit in ASTMap for an instantiation that comes later.
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
      const auto ModName = enterModule(Cmd);

      if (Cmd.DefinitionName) {
        auto ASTDef = ASTMap.find(std::string(*Cmd.DefinitionName));
        if (ASTDef != ASTMap.end()) {
          if (auto Res = onInstantiate(RootCtx, ModName, ASTDef->second)) {
            // The command is successful.
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
      // The preprocessing step already did this command.
      break;
    }
    case Wast::CommandType::Action: {
      if (IsComponent) {
        break;
      }
      if (!Cmd.Act) {
        break;
      }
      // A bare action fails the unit if it does not succeed, so it counts
      // as an assertion.
      const auto &Act = *Cmd.Act;
      const auto ModName = GetModuleName(Act);
      if (Act.Type == Wast::ActionType::Invoke) {
        if (auto Res = onInvoke(RootCtx, ModName, Act.FieldName, Act.Args,
                                Act.ArgTypes)) {
          // The command is successful.
        } else {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line) << "Action invoke failed";
        }
      } else {
        if (auto Res = onGet(RootCtx, ModName, Act.FieldName)) {
          // The command is successful.
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
        auto Res =
            onInvoke(RootCtx, ModName, Act.FieldName, Act.Args, Act.ArgTypes);
        if (!Res) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "assert_return invoke failed: "
              << WasmEdge::ErrCodeStr[Res.error().getEnum()];
          break;
        }
        EXPECT_TRUE(compareResults(Cmd.Expected, *Res))
            << TestFile << ":" << Cmd.Line << ": assert_return mismatch";
      } else {
        auto Res = onGet(RootCtx, ModName, Act.FieldName);
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
        auto Res =
            onInvoke(RootCtx, ModName, Act.FieldName, Act.Args, Act.ArgTypes);
        if (Res) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "assert_trap expected trap";
        } else {
          EXPECT_TRUE(Res.error().getErrCodePhase() ==
                      WasmEdge::WasmPhase::Execution)
              << TestFile << ":" << Cmd.Line << ": assert_trap wrong phase";
          EXPECT_TRUE(stringContains(
              Cmd.ExpectedMessage, WasmEdge::ErrCodeStr[Res.error().getEnum()]))
              << TestFile << ":" << Cmd.Line
              << ": assert_trap message mismatch";
        }
      } else if (!Cmd.ModuleSource.empty()) {
        // The module traps at the instantiation. Run onParse, then onValidate,
        // then onInstantiate.
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
          if (!Cmd.ExpectedMessage.empty()) {
            EXPECT_TRUE(
                stringContains(Cmd.ExpectedMessage,
                               WasmEdge::ErrCodeStr[InstRes.error().getEnum()]))
                << TestFile << ":" << Cmd.Line
                << ": assert_trap message mismatch";
          }
        }
      }
      break;
    }
    case Wast::CommandType::AssertExhaustion: {
      if (!Cmd.Act) {
        break;
      }
      if (IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Execution)) {
        break;
      }
      const auto &Act = *Cmd.Act;
      const auto ModName = GetModuleName(Act);
      auto Res =
          onInvoke(RootCtx, ModName, Act.FieldName, Act.Args, Act.ArgTypes);
      if (Res) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_exhaustion expected call stack exhaustion";
      } else {
        EXPECT_EQ(Res.error(), WasmEdge::ErrCode::Value::CallStackExhausted)
            << TestFile << ":" << Cmd.Line << ": assert_exhaustion wrong error";
      }
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

      // The phase test is strict for a text module too. The converter reports
      // only the syntax errors, and the validator reports the rest, as with a
      // binary module. This test finds a wrong class of error from the loader
      // or from the validator.

      // Run onParse.
      auto ParseRes = onParse(RootCtx, Source, Type, Conf);
      if (!ParseRes) {
        const auto Phase = ParseRes.error().getErrCodePhase();
        EXPECT_TRUE(Phase == WasmEdge::WasmPhase::Validation)
            << TestFile << ":" << Cmd.Line << ": assert_invalid wrong phase: "
            << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
        if (!Cmd.ExpectedMessage.empty()) {
          EXPECT_TRUE(
              stringContains(Cmd.ExpectedMessage,
                             WasmEdge::ErrCodeStr[ParseRes.error().getEnum()]))
              << TestFile << ":" << Cmd.Line
              << ": assert_invalid message mismatch";
        }
        break;
      }

      // Run onValidate. This step must fail.
      auto ValRes = onValidate(RootCtx, *ParseRes);
      if (ValRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_invalid expected failure";
      } else {
        const auto Phase = ValRes.error().getErrCodePhase();
        EXPECT_TRUE(Phase == WasmEdge::WasmPhase::Validation)
            << TestFile << ":" << Cmd.Line << ": assert_invalid wrong phase: "
            << WasmEdge::ErrCodeStr[ValRes.error().getEnum()];
        if (!Cmd.ExpectedMessage.empty()) {
          EXPECT_TRUE(
              stringContains(Cmd.ExpectedMessage,
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

      // The phase test is strict for a text module too. This test finds a
      // wrong class of error from the loader or from the validator.

      // Run only onParse. This step must fail.
      auto ParseRes = onParse(RootCtx, Source, Type, Conf);
      if (ParseRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_malformed expected failure";
      } else {
        const auto Phase = ParseRes.error().getErrCodePhase();
        EXPECT_TRUE(Phase == WasmEdge::WasmPhase::Loading)
            << TestFile << ":" << Cmd.Line << ": assert_malformed wrong phase: "
            << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
        if (!Cmd.ExpectedMessage.empty()) {
          EXPECT_TRUE(
              stringContains(Cmd.ExpectedMessage,
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

      // Run onParse, then onValidate, then onInstantiate. The error must come
      // from the instantiation.
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
            stringContains(Cmd.ExpectedMessage,
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
        auto Res =
            onInvoke(RootCtx, ModName, Act.FieldName, Act.Args, Act.ArgTypes);
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
      if (Cmd.ThreadName) {
        ThreadName = std::string(*Cmd.ThreadName);
      }

      if (ThreadMap.find(ThreadName) != ThreadMap.end()) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Duplicate thread name: " << ThreadName;
        break;
      }

      // Scan the sub-commands of the thread for the register entries first.
      // These entries give the alias names of the shared modules.
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
          ThreadName,
          std::thread([this, ChildCtx, SubCmds = Cmd.SubCommands, Conf,
                       IsComponent, TestFile, &ThreadResultMutex,
                       &ThreadResults]() mutable {
            testing::TestPartResultArray Results;
            {
              testing::ScopedFakeTestPartResultReporter Reporter(
                  testing::ScopedFakeTestPartResultReporter::
                      INTERCEPT_ONLY_CURRENT_THREAD,
                  &Results);
              executeCommands(SubCmds, Conf, IsComponent, ChildCtx, TestFile);
              onFini(ChildCtx);
            }
            std::lock_guard<std::mutex> Lock(ThreadResultMutex);
            for (int I = 0; I < Results.size(); ++I) {
              ThreadResults.push_back(Results.GetTestPartResult(I));
            }
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
  // For safety, join each thread that no command waited for.
  for (auto &[Name, Thread] : ThreadMap) {
    if (Thread.joinable()) {
      Thread.join();
    }
  }

  // Every new thread joined. Report their assertion failures again on this
  // thread.
  {
    std::lock_guard<std::mutex> Lock(ThreadResultMutex);
    for (const auto &R : ThreadResults) {
      if (R.failed()) {
        const char *File = R.file_name() ? R.file_name() : TestFile.c_str();
        ADD_FAILURE_AT(File, R.line_number()) << R.message();
      }
    }
    ThreadResults.clear();
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
      // A JSON file that does not parse is a failure, and not a skip. A skip
      // hides a corrupt test unit behind a green result.
      ADD_FAILURE_AT(u8string(JsonPath).c_str(), 1)
          << "Failed to parse JSON: " << u8string(JsonPath);
      onFini(RootCtx);
      return;
    }
    executeCommands(ScriptResult->Commands, Conf, IsComp, RootCtx,
                    u8string(JsonPath));
  } else {
    spdlog::debug("{} {}"sv, Proposal, UnitName);
    auto WastPath =
        TestsuiteRoot / ProposalPath / UName / (std::string(UName) + ".wast"s);
    auto ScriptResult = Wast::parseWast(WastPath);
    if (!ScriptResult) {
      spdlog::error("Failed to parse WAST file: {}"sv, u8string(WastPath));
      ADD_FAILURE_AT(u8string(WastPath).c_str(), 1)
          << "Failed to parse WAST: " << u8string(WastPath);
      onFini(RootCtx);
      return;
    }
    executeCommands(ScriptResult->Commands, Conf, IsComp, RootCtx,
                    u8string(WastPath));
  }
  onFini(RootCtx);
}

} // namespace WasmEdge

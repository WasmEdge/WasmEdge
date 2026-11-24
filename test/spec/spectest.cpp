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
#include "loader/loader.h"
#include "runtime/instance/composite.h"
#include "runtime/instance/module.h"
#include "wast_parser.h"
#include "wat/parser.h"
#include "wat/wat_util.h"

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace WasmEdge {
thread_local bool SpecTest::SkipComponentValidation = false;
}

namespace {

using namespace std::literals;
using namespace WasmEdge;

// The canonical NaN bit patterns of f32 and f64, with a clear sign bit.
constexpr uint32_t CanonicalNaN32 = UINT32_C(0x7FC00000);
constexpr uint64_t CanonicalNaN64 = UINT64_C(0x7FF8000000000000);

// Match the bits Got of a float against the NaN pattern NaN, which is not
// None. A canonical NaN must have the bits CanonicalBits apart from the sign
// bit. An arithmetic NaN must have the exponent bits and the top payload bit
// of CanonicalBits, so a signaling NaN does not match. The other payload bits
// and the sign can have any value.
template <typename BitsT>
bool matchNaN(BitsT Got, Wast::Result::NaNPattern NaN, BitsT CanonicalBits) {
  constexpr BitsT SignMask = static_cast<BitsT>(~BitsT{0} >> 1);
  if (NaN == Wast::Result::NaNPattern::Canonical) {
    return (Got & SignMask) == CanonicalBits;
  }
  return (Got & CanonicalBits) == CanonicalBits;
}

// Compare the float lanes of two V128 values. A lane with a NaN pattern
// matches any NaN of that pattern, and the other lanes match bit for bit.
template <typename BitsT>
bool compareFloatLanes(const uint128_t &GotV, const uint128_t &ExpV,
                       const std::vector<Wast::Result::NaNPattern> &LaneNaN,
                       BitsT CanonicalBits) {
  constexpr size_t LaneCount = sizeof(uint128_t) / sizeof(BitsT);
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
    if (NaN == Wast::Result::NaNPattern::None
            ? GotBits[I] != ExpBits[I]
            : !matchNaN(GotBits[I], NaN, CanonicalBits)) {
      return false;
    }
  }
  return true;
}

// The hierarchy of a reference. Two references can match only in the same
// hierarchy. All null references of one hierarchy are the same value.
enum class RefHierarchy { None, Any, Func, Extern, Exn };

RefHierarchy hierarchyOf(TypeCode Code) noexcept {
  switch (Code) {
  case TypeCode::AnyRef:
  case TypeCode::EqRef:
  case TypeCode::I31Ref:
  case TypeCode::StructRef:
  case TypeCode::ArrayRef:
  case TypeCode::NullRef:
    return RefHierarchy::Any;
  case TypeCode::FuncRef:
  case TypeCode::NullFuncRef:
    return RefHierarchy::Func;
  case TypeCode::ExternRef:
  case TypeCode::NullExternRef:
    return RefHierarchy::Extern;
  case TypeCode::ExnRef:
  case TypeCode::NullExnRef:
    return RefHierarchy::Exn;
  default:
    return RefHierarchy::None;
  }
}

// Give the runtime heap type of the reference R, which is not null. A struct,
// an array, or a function with a defined type holds its type index, so the
// composite type of its module gives StructRef, ArrayRef, or FuncRef. The
// other references hold an abstract heap type, such as I31Ref or ExternRef. A
// converted reference keeps the heap type of its value.
TypeCode runtimeHeapType(const RefVariant &R) noexcept {
  const auto Code = R.getType().getHeapTypeCode();
  if (Code != TypeCode::TypeIndex) {
    return Code;
  }
  const auto *Inst = R.getPtr<const Runtime::Instance::CompositeBase>();
  const auto *ModInst = Inst->getModule();
  if (ModInst == nullptr) {
    return TypeCode::FuncRef;
  }
  const auto Type = ModInst->getType(Inst->getTypeIndex());
  if (!Type) {
    return TypeCode::Epsilon;
  }
  switch ((*Type)->getCompositeType().getContentTypeCode()) {
  case TypeCode::Func:
    return TypeCode::FuncRef;
  case TypeCode::Struct:
    return TypeCode::StructRef;
  case TypeCode::Array:
    return TypeCode::ArrayRef;
  default:
    return TypeCode::Epsilon;
  }
}

// Give the hierarchy of the reference Got. The result type gives it, except
// for a type index, where the value gives it. A null value holds the bottom
// type of its hierarchy.
RefHierarchy hierarchyOf(const std::pair<ValVariant, ValType> &Got) noexcept {
  if (const auto H = hierarchyOf(Got.second.getHeapTypeCode());
      H != RefHierarchy::None) {
    return H;
  }
  const auto &R = Got.first.get<RefVariant>();
  return hierarchyOf(R.isNull() ? R.getType().getHeapTypeCode()
                                : runtimeHeapType(R));
}

// Tell if a reference that is not null, with the runtime heap type Kind, fits
// the expected heap type Expected of the same hierarchy. An abstract top type
// fits every reference of its hierarchy. A bottom type fits no reference that
// is not null.
bool fitsHeapType(TypeCode Expected, TypeCode Kind) noexcept {
  switch (Expected) {
  case TypeCode::AnyRef:
  case TypeCode::FuncRef:
  case TypeCode::ExternRef:
  case TypeCode::ExnRef:
    return true;
  case TypeCode::EqRef:
    return Kind == TypeCode::I31Ref || Kind == TypeCode::StructRef ||
           Kind == TypeCode::ArrayRef;
  case TypeCode::I31Ref:
  case TypeCode::StructRef:
  case TypeCode::ArrayRef:
    return Kind == Expected;
  default:
    return false;
  }
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
    {"wide-arithmetic"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::WideArithmetic}},
    // Currently, the component model supports only interpreter mode.
    {"component-model-async"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"component-model-binary"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"component-model-linking"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"component-model-resources"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"component-model-validation"sv,
     WasmEdge::Standard::WASM_3,
     {Proposal::Component, Proposal::Threads},
     {},
     WasmEdge::SpecTest::TestMode::Interpreter},
    {"component-model-values"sv,
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
};

// The folder-name prefix of the component-model suites.
constexpr std::string_view ComponentModelPrefix = "component-model-"sv;

// Whether the folder holds component-model suites.
bool isComponentModel(std::string_view Proposal) {
  return Proposal.substr(0, ComponentModelPrefix.size()) ==
         ComponentModelPrefix;
}

// clang-format off
// This map gives the component model support of each test suite.
// Delete this map after the component model is complete.
std::unordered_map<std::string, ComponentModelSupport, Hash::Hash>
    ComponentModelFolders = {
    // | Suite | Test table: {load, validate, instantiate, execute} |
    // ---------------------------------------------------------------
    // Suite: the folder name without the `component-model-` prefix, then the
    //        directory name of the test.
    // Test table: the testing status of load, validate, instantiate, and execute.
    {"async/async-calls-sync",                                   {true, true, false, false}},
    {"async/big-interleaving-test",                              {true, true, false, false}},
    {"async/builtin-trap-poisons-instance",                      {true, true, false, false}},
    {"async/cancel-and-exclusive-lock",                          {true, true, false, false}},
    {"async/cancel-delivery",                                    {true, true, false, false}},
    {"async/cancel-not-delivered",                               {true, true, false, false}},
    {"async/cancel-resumed-callback-switch",                     {true, true, false, false}},
    {"async/cancel-stream",                                      {true, true, false, false}},
    {"async/cancel-subtask",                                     {true, true, false, false}},
    {"async/cancel-targeted-resume",                             {true, true, false, false}},
    {"async/closed-stream",                                      {true, true, false, false}},
    {"async/cross-abi-calls",                                    {true, true, false, false}},
    {"async/cross-task-future",                                  {true, true, false, false}},
    {"async/deadlock",                                           {true, true, false, false}},
    {"async/dont-block-start",                                   {true, true, false, false}},
    {"async/drop-cross-task-borrow",                             {true, true, false, false}},
    {"async/drop-stream",                                        {true, true, false, false}},
    {"async/drop-subtask",                                       {true, true, false, false}},
    {"async/drop-waitable-set",                                  {true, true, false, false}},
    {"async/dtor-context-is-zero",                               {true, true, false, false}},
    {"async/during-sync-call-blocking-ops",                      {true, true, false, false}},
    {"async/during-sync-call-exclusive-resume",                  {true, true, false, false}},
    {"async/during-sync-call-may-block-if-other-ready-threads",  {true, true, false, false}},
    {"async/during-sync-call-no-sibling-resume",                 {true, true, false, false}},
    {"async/during-sync-scheduling-candidates",                  {true, true, false, false}},
    {"async/empty-wait",                                         {true, true, false, false}},
    {"async/exit-without-resolving",                             {true, true, false, false}},
    {"async/forward",                                            {true, true, false, false}},
    {"async/future-completion-order",                            {true, true, false, false}},
    {"async/futures-must-write",                                 {true, true, false, false}},
    {"async/idle-drop",                                          {true, true, false, false}},
    {"async/partial-stream-copies",                              {true, true, false, false}},
    {"async/passing-resources",                                  {true, true, false, false}},
    {"async/reentrance",                                         {true, true, false, false}},
    {"async/same-component-stream-future",                       {true, true, false, false}},
    {"async/self-switch-traps",                                  {true, true, false, false}},
    {"async/switch-to-ready-callback",                           {true, true, false, false}},
    {"async/sync-barges-in",                                     {true, true, false, false}},
    {"async/sync-streams",                                       {true, true, false, false}},
    {"async/trap-if-block-and-sync",                             {true, true, false, false}},
    {"async/trap-if-done",                                       {true, true, false, false}},
    {"async/trap-if-sync-and-waitable-set",                      {true, true, false, false}},
    {"async/trap-if-transfer-in-waitable-set",                   {true, true, false, false}},
    {"async/validate-no-async-abi-for-sync-type",                {true, true, true, true}},
    {"async/validate-no-stream-char",                            {true, true, true, true}},
    {"async/validate-task-return",                               {true, true, false, false}},
    {"async/wait-during-callback",                               {true, true, false, false}},
    {"async/zero-length",                                        {true, true, false, false}},
    {"binary/binary",                                            {true, true, false, false}},
    {"linking/link-time-virtualization",                         {true, true, false, false}},
    {"linking/shared-everything-dynamic-linking",                {true, true, false, false}},
    {"linking/tags",                                             {true, true, false, false}},
    {"linking/unit",                                             {true, true, false, false}},
    {"resources/borrows",                                        {true, true, false, false}},
    {"resources/handle-table",                                   {true, true, false, false}},
    {"resources/multiple-resources",                             {true, true, false, false}},
    {"validation/abi",                                           {true, true, true, true}},
    {"validation/annotated-names",                               {true, true, true, true}},
    {"validation/attributes",                                    {true, true, true, true}},
    {"validation/core-modules",                                  {true, true, true, true}},
    {"validation/defined-types",                                 {true, true, true, true}},
    {"validation/extern-names",                                  {true, true, true, true}},
    {"validation/external-visibility",                           {true, true, true, true}},
    {"validation/indicies",                                      {true, true, true, true}}, // codespell:ignore indicies
    {"validation/instantiation",                                 {true, true, false, false}},
    {"validation/kebab",                                         {true, true, true, true}},
    {"validation/map",                                           {true, true, true, true}},
    {"validation/max-value-size",                                {true, true, true, true}},
    {"validation/outer-alias",                                   {true, true, true, true}},
    {"validation/resources",                                     {true, true, false, false}},
    {"validation/version-suffix",                                {true, true, true, true}},
    {"values/alignment",                                         {true, true, false, false}},
    {"values/concat",                                            {true, true, false, false}},
    {"values/numerics",                                          {true, true, false, false}},
    {"values/post-return",                                       {true, true, false, false}},
    {"values/realloc",                                           {true, true, false, false}},
    {"values/strings",                                           {true, true, true, false}},
    {"values/transcode",                                         {true, true, false, false}},
    {"values/variants",                                          {true, true, false, false}},
};
// clang-format on

// Get the component model support of one test suite. The suite is the
// folder name without the `component-model-` prefix, then a slash and the
// directory name of the test.
// Delete this function after the component model is complete.
bool checkComponentSupported(std::string_view Suite, WasmEdge::WasmPhase P) {
  auto It = ComponentModelFolders.find(std::string(Suite));
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
    if (!IncludeComponent && isComponentModel(Proposal.Path)) {
      continue;
    }
    // The WAST mode does not support the component-model tests.
    if (this->Mode == ParserMode::Wast && isComponentModel(Proposal.Path)) {
      continue;
    }
    const std::filesystem::path ProposalRoot = TestsuiteRoot / Proposal.Path;
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
    // This is an opaque reference. Every value of the type that is not null is
    // correct.
    if (Expected.OpaqueRef) {
      return !R.isNull();
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
    if (Expected.NaN != Wast::Result::NaNPattern::None) {
      return matchNaN(Got.first.get<uint32_t>(), Expected.NaN, CanonicalNaN32);
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
    if (Expected.NaN != Wast::Result::NaNPattern::None) {
      return matchNaN(Got.first.get<uint64_t>(), Expected.NaN, CanonicalNaN64);
    }
    return Got.first.get<uint64_t>() == Expected.Value.get<uint64_t>();
  default:
    break;
  }

  // These are the reference types. A reference matches only in its own
  // hierarchy. For a value that is not null, the runtime heap type must also
  // fit the expected heap type, so a struct does not match (ref.func) and a
  // function does not match (ref.struct).
  if (Expected.Type.isRefType()) {
    if (!Got.second.isRefType()) {
      return false;
    }
    const auto &R = Got.first.get<RefVariant>();
    // A bare (ref.null) matches every null reference.
    if (Expected.AnyNullRef) {
      return R.isNull();
    }
    const auto GotHierarchy = hierarchyOf(Got);
    // A bare (ref) matches every reference of the any hierarchy that is not
    // null.
    if (Expected.AnyNonNullRef) {
      return !R.isNull() && GotHierarchy == RefHierarchy::Any;
    }
    if (GotHierarchy == RefHierarchy::None ||
        GotHierarchy != hierarchyOf(HTCode)) {
      return false;
    }
    if (!R.isNull() && !fitsHeapType(HTCode, runtimeHeapType(R))) {
      return false;
    }
    return IsRefMatch(R);
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
      return compareFloatLanes<uint32_t>(GotV, ExpV, Expected.V128LaneNaN,
                                         CanonicalNaN32);
    }
    if (Expected.V128Shape == "f64x2"sv) {
      return compareFloatLanes<uint64_t>(GotV, ExpV, Expected.V128LaneNaN,
                                         CanonicalNaN64);
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
  std::unordered_set<size_t> RegisteredAnonCmds;
  // These are the module definitions for the module instance commands. A
  // definition command and a plain module command both make a definition.
  // The empty name holds the latest definition, as in the reference
  // interpreter. The runner keeps a definition only if the script has a
  // module instance command.
  std::unordered_map<std::string, std::shared_ptr<SpecTest::WasmUnit>,
                     Hash::Hash>
      Definitions;
  std::string LastModName;
  // This map gives the store name of the latest instance of each script id.
  // A module command with an id that is already bound binds the id again, as
  // in the reference interpreter.
  std::unordered_map<std::string, std::string, Hash::Hash> Bindings;
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

  // This is the suite name for checkComponentSupported(). A key of
  // ComponentModelFolders is the proposal folder without its prefix, a slash,
  // and the unit name with no extension. For example,
  // ".../component-model-values/strings/strings.json" gives "values/strings".
  std::string UnitName;
  if (IsComponent) {
    const auto PF = u8path(TestFile);
    const auto Proposal = u8string(PF.parent_path().parent_path().filename());
    UnitName = Proposal.substr(ComponentModelPrefix.size()) + '/' +
               u8string(PF.stem());
  }

  // Resolve the source for onParse. For a module that comes from a file, this
  // lambda resolves the full path and stores it in ResolvedPaths as UTF-8. The
  // stored path then holds the bytes of the view that the lambda returns, and
  // a reader converts it back with u8path.
  auto ResolveSource = [&](const Wast::ScriptCommand &Cmd)
      -> std::pair<std::string_view, Wast::ModuleType> {
    if (Cmd.ModType == Wast::ModuleType::TextFile ||
        Cmd.ModType == Wast::ModuleType::BinaryFile) {
      auto Path = u8path(TestFile).parent_path() / u8path(Cmd.ModuleSource);
      ResolvedPaths.push_back(u8string(Path));
      return {ResolvedPaths.back(), Cmd.ModType};
    }
    return {Cmd.ModuleSource, Cmd.ModType};
  };

  // The position of the command Cmd in Commands. It identifies a command,
  // also when two commands are on one line.
  auto CmdIndex = [&Commands](const Wast::ScriptCommand &Cmd) {
    return static_cast<size_t>(&Cmd - Commands.data());
  };

  // This is the preprocessing step. It finds the anonymous modules that a
  // register command names, so that their instances can live in the store
  // under a name of the runner. A module definition creates no instance, so it
  // is not the target of a register.
  bool HasInstanceCmd = false;
  {
    std::optional<size_t> LastAnonCmd;
    for (const auto &Cmd : Commands) {
      HasInstanceCmd |=
          Cmd.Type == Wast::CommandType::ModuleInstance || Cmd.InstanceOperand;
      if (Cmd.Type == Wast::CommandType::Module ||
          Cmd.Type == Wast::CommandType::ModuleInstance) {
        LastAnonCmd = Cmd.ModuleName ? std::nullopt
                                     : std::optional<size_t>(CmdIndex(Cmd));
      } else if (Cmd.Type == Wast::CommandType::Register && !Cmd.ModuleName &&
                 LastAnonCmd) {
        RegisteredAnonCmds.insert(*LastAnonCmd);
      }
    }
  }

  // The store names of the runner start with the byte 0xFF. A name of the
  // script is valid UTF-8, and valid UTF-8 never holds the byte 0xFF, so a
  // register name of the script cannot be equal to a name of the runner.
  constexpr std::string_view RunnerPrefix = "\xFF"sv;

  // A module that a thread shares has the name "\xFF$<id>" in the store of
  // the thread. The WAST parser gives the decoded id, such as "m" for $m, and
  // the JSON parser gives "$m". Both forms get the prefix, so each id has one
  // name.
  auto SharedName = [RunnerPrefix](std::string_view Id) {
    return std::string(RunnerPrefix) + "$" + std::string(Id);
  };

  // This lambda gives the store name of the instance that the script id Id
  // names. An id with no binding is a shared module of a thread.
  auto StoreName = [&](std::string_view Id) {
    if (auto It = Bindings.find(std::string(Id)); It != Bindings.end()) {
      return It->second;
    }
    return SharedName(Id);
  };

  // This lambda gives the store name of a module command. A named module, and
  // an anonymous module that a register names, get the name
  // "\xFF#<command index>". Each instance thus has its own name, also when
  // two modules have the same id. Another anonymous module has no name and
  // becomes the active module. The name becomes the target of the actions
  // that do not name a module.
  auto EnterModule = [&](const Wast::ScriptCommand &Cmd) -> std::string {
    std::string ModName;
    if (Cmd.ModuleName || RegisteredAnonCmds.count(CmdIndex(Cmd)) != 0) {
      ModName = std::string(RunnerPrefix) + "#" + std::to_string(CmdIndex(Cmd));
    }
    LastModName = ModName;
    return ModName;
  };

  // Bind the id of the command Cmd to the store name ModName after a
  // successful instantiation.
  auto BindModule = [&](const Wast::ScriptCommand &Cmd,
                        const std::string &ModName) {
    if (Cmd.ModuleName) {
      Bindings[std::string(*Cmd.ModuleName)] = ModName;
    }
  };

  // This lambda resolves the module name of an action.
  auto GetModuleName = [&](const Wast::Action &Act) -> std::string {
    if (Act.ModuleName) {
      return StoreName(*Act.ModuleName);
    }
    return LastModName;
  };

  // Store the unit Unit as the latest definition, and also under the id of
  // the command Cmd if it has one.
  auto AddDefinition = [&](const Wast::ScriptCommand &Cmd, WasmUnit &&Unit) {
    if (!HasInstanceCmd) {
      return;
    }
    auto Def = std::make_shared<WasmUnit>(std::move(Unit));
    if (Cmd.ModuleName) {
      Definitions[std::string(*Cmd.ModuleName)] = Def;
    }
    Definitions[std::string()] = std::move(Def);
  };

  // Run the action Act. An invoke calls a function, and a get reads a
  // global. The value of a get is the one value of the result list.
  auto RunAction = [&](const Wast::Action &Act)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    const auto ModName = GetModuleName(Act);
    if (Act.Type == Wast::ActionType::Get) {
      EXPECTED_TRY(auto Value, onGet(RootCtx, ModName, Act.FieldName));
      return std::vector<std::pair<ValVariant, ValType>>{std::move(Value)};
    }
    return onInvoke(RootCtx, ModName, Act.FieldName, Act.Args, Act.ArgTypes);
  };

  // Run the action of an assertion that expects the action to fail, and give
  // the error. A get cannot trap, so a get that fails is an error of the
  // script, and it never satisfies the assertion. Each failure is reported
  // here, and the result is then empty.
  auto RunFailingAction = [&](const Wast::ScriptCommand &Cmd,
                              std::string_view What) -> std::optional<ErrCode> {
    auto Res = RunAction(*Cmd.Act);
    if (Res) {
      ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line) << What << " expected failure";
      return std::nullopt;
    }
    if (Cmd.Act->Type == Wast::ActionType::Get) {
      ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
          << What
          << " get failed: " << WasmEdge::ErrCodeStr[Res.error().getEnum()];
      return std::nullopt;
    }
    return Res.error();
  };

  // Parse a module in the format that the script declares. A loader with WAT
  // input finds the format from the bytes, so the harness handles the two
  // cases where the bytes look like the other format. Binary bytes that look
  // like text go to a loader without WAT input, which rejects them as a
  // binary. Text that does not look like text goes to the WAT parser. Every
  // other source goes to onParse.
  //
  // The first byte of a file fixture decides the result of maybeWAT if the
  // byte is '(' or a byte that cannot start text, such as the NUL of the
  // binary magic. Most fixtures start with such a byte, and the harness then
  // reads only this byte. The runners read the file by its path, because an
  // AOT runner compiles the file. The harness reads the whole file only if the
  // first byte does not decide, or if the bytes look like the other format. A
  // fixture that the harness cannot read is an error of the fixture, and not
  // an empty module.
  auto FirstByteLooksLikeText = [](int C) -> std::optional<bool> {
    if (C == std::char_traits<char>::eof()) {
      return false;
    }
    const auto B = static_cast<unsigned char>(C);
    if (B == '(') {
      return true;
    }
    if (WAT::isWatSpace(B) || B == ';' || B == 0xEF) {
      return std::nullopt;
    }
    return false;
  };

  auto ParseSource = [&](std::string_view Source,
                         Wast::ModuleType Type) -> Expect<WasmUnit> {
    const bool IsFile = Type == Wast::ModuleType::TextFile ||
                        Type == Wast::ModuleType::BinaryFile;
    const bool IsText =
        Type == Wast::ModuleType::TextFile || Type == Wast::ModuleType::Text;
    std::string FileBytes;
    if (IsFile) {
      std::ifstream File(u8path(Source), std::ios::binary);
      if (!File) {
        return Unexpect(ErrCode::Value::IllegalPath);
      }
      const auto FirstByteText = FirstByteLooksLikeText(File.peek());
      if (File.bad()) {
        return Unexpect(ErrCode::Value::IllegalPath);
      }
      if (FirstByteText == IsText) {
        return onParse(RootCtx, Source, Type, Conf);
      }
      FileBytes.assign(std::istreambuf_iterator<char>(File),
                       std::istreambuf_iterator<char>());
      if (File.bad()) {
        return Unexpect(ErrCode::Value::IllegalPath);
      }
    }
    const std::string_view Bytes =
        IsFile ? std::string_view(FileBytes) : Source;
    const bool LooksLikeText = WAT::maybeWAT(Span<const uint8_t>(
        reinterpret_cast<const uint8_t *>(Bytes.data()), Bytes.size()));
    if (!IsText && LooksLikeText) {
      Configure BinaryConf(Conf);
      BinaryConf.setEnableWAT(false);
      Loader::Loader Ld(BinaryConf);
      EXPECTED_TRY(
          auto Mod,
          Ld.parseModule(Span<const uint8_t>(
              reinterpret_cast<const uint8_t *>(Bytes.data()), Bytes.size())));
      return WasmUnit(std::move(Mod));
    }
    if (IsText && !LooksLikeText) {
      EXPECTED_TRY(auto Mod, WAT::parseWat(Bytes, Conf));
      return WasmUnit(std::make_unique<AST::Module>(std::move(Mod)));
    }
    return onParse(RootCtx, Source, Type, Conf);
  };

  // Check that the error E of the assertion What comes from one of the phases
  // Phases, and that its text is in the expected message of Cmd. A fixture
  // that cannot be read never satisfies an assertion, also if the expected
  // message is empty.
  auto ExpectError = [this, &TestFile](const Wast::ScriptCommand &Cmd,
                                       const ErrCode &E,
                                       std::initializer_list<WasmPhase> Phases,
                                       std::string_view What) {
    const auto Str = WasmEdge::ErrCodeStr[E.getEnum()];
    if (E.getEnum() == ErrCode::Value::IllegalPath) {
      ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
          << What << " cannot read its module fixture: " << Str;
      return;
    }
    EXPECT_NE(std::find(Phases.begin(), Phases.end(), E.getErrCodePhase()),
              Phases.end())
        << TestFile << ":" << Cmd.Line << ": " << What
        << " wrong phase: " << Str;
    if (!Cmd.ExpectedMessage.empty()) {
      EXPECT_TRUE(stringContains(Cmd.ExpectedMessage, Str))
          << TestFile << ":" << Cmd.Line << ": " << What << " message mismatch";
    }
  };

  // Find the definition of the module instance Cmd. A module instance with no
  // definition id uses the latest definition. A definition that is not found
  // is reported here, and the result is then null.
  auto FindDefinition =
      [&](const Wast::ScriptCommand &Cmd,
          std::string_view What) -> std::shared_ptr<WasmUnit> {
    const std::string DefName(Cmd.DefinitionName.value_or(""sv));
    auto Def = Definitions.find(DefName);
    if (Def == Definitions.end()) {
      ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
          << What << " module definition not found: "
          << (DefName.empty() ? "(latest)"s : DefName);
      return nullptr;
    }
    return Def->second;
  };

  // Give the unit that the assertion Cmd instantiates. A module of the
  // assertion must parse and validate. A module instance operand takes its
  // definition, which already passed the validation. Each failure is reported
  // here, and the result is then null.
  auto AssertionUnit = [&](const Wast::ScriptCommand &Cmd,
                           std::string_view What) -> std::shared_ptr<WasmUnit> {
    if (Cmd.InstanceOperand) {
      return FindDefinition(Cmd, What);
    }
    auto [Source, Type] = ResolveSource(Cmd);
    auto ParseRes = ParseSource(Source, Type);
    if (!ParseRes) {
      ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
          << What << " parse unexpectedly failed: "
          << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
      return nullptr;
    }
    if (auto ValRes = onValidate(RootCtx, *ParseRes); !ValRes) {
      ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
          << What << " validate unexpectedly failed: "
          << WasmEdge::ErrCodeStr[ValRes.error().getEnum()];
      return nullptr;
    }
    // The script grammar makes an inline module an anonymous definition before
    // the assertion, so the module becomes the latest definition. The latest
    // instance of the unnamed actions does not change.
    auto Unit = std::make_shared<WasmUnit>(std::move(*ParseRes));
    if (HasInstanceCmd) {
      Definitions[std::string()] = Unit;
    }
    return Unit;
  };

  // Instantiate the unit of the assertion Cmd, and check that it fails in the
  // phase Phase. A link error comes from the instantiation, and a trap of a
  // start function or of a segment initialization comes from the execution.
  // The assertion What then cannot pass with an error of the other kind.
  auto ExpectInstantiationError = [&](const Wast::ScriptCommand &Cmd,
                                      WasmPhase Phase, std::string_view What) {
    auto Unit = AssertionUnit(Cmd, What);
    if (!Unit) {
      return;
    }
    if (auto InstRes = onInstantiate(RootCtx, "", *Unit); InstRes) {
      ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line) << What << " expected failure";
    } else {
      ExpectError(Cmd, InstRes.error(), {Phase}, What);
    }
  };

  // Run the commands.
  for (const auto &Cmd : Commands) {
    switch (Cmd.Type) {
    case Wast::CommandType::Module: {
      const auto ModName = EnterModule(Cmd);

      // Clear this flag for each command. Only a component with no
      // validation support sets it.
      SkipComponentValidation = false;

      if (IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Instantiation)) {
        // The instantiation has no support. Do the validate step or the load
        // step.
        auto [Source, Type] = ResolveSource(Cmd);
        auto ParseRes = ParseSource(Source, Type);
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

      auto [Source, Type] = ResolveSource(Cmd);

      // Run onParse.
      auto ParseRes = ParseSource(Source, Type);
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
        break;
      }
      BindModule(Cmd, ModName);
      // A plain module is also a definition. A component unit can belong to
      // the context now, and a component has no module instance command.
      if (!IsComponent) {
        AddDefinition(Cmd, std::move(*ParseRes));
      }
      break;
    }
    case Wast::CommandType::ModuleDefinition: {
      // A definition creates no instance, so it does not change the target of
      // the actions that do not name a module.

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

      auto [Source, Type] = ResolveSource(Cmd);

      // Run onParse.
      auto ParseRes = ParseSource(Source, Type);
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

      // Store the unit for an instantiation that comes later.
      AddDefinition(Cmd, std::move(*ParseRes));
      break;
    }
    case Wast::CommandType::ModuleInstance: {
      if (IsComponent) {
        // Instantiating a component from a definition is not supported yet:
        // skip it unless the suite expects instantiation to work.
        if (checkComponentSupported(UnitName, WasmPhase::Instantiation)) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "Unexpected module_instance for component";
        }
        break;
      }
      auto Def = FindDefinition(Cmd, "module instance"sv);
      if (!Def) {
        break;
      }
      const auto ModName = EnterModule(Cmd);
      if (auto Res = onInstantiate(RootCtx, ModName, *Def); !Res) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Module instance failed: "
            << WasmEdge::ErrCodeStr[Res.error().getEnum()];
        break;
      }
      BindModule(Cmd, ModName);
      break;
    }
    case Wast::CommandType::Register: {
      // Add the register name to the instance now. An import before this
      // command does not see the name, and a module can take more than one
      // name.
      if (Cmd.RegisterName.find(RunnerPrefix) != std::string_view::npos) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Register name is not valid UTF-8";
        break;
      }
      const auto ModName =
          Cmd.ModuleName ? StoreName(*Cmd.ModuleName) : LastModName;
      if (auto Res =
              onRegister(RootCtx, ModName, std::string(Cmd.RegisterName));
          !Res) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Register failed: "
            << WasmEdge::ErrCodeStr[Res.error().getEnum()];
      }
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
      if (auto Res = RunAction(*Cmd.Act); !Res) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "Action failed: " << WasmEdge::ErrCodeStr[Res.error().getEnum()];
      }
      break;
    }
    case Wast::CommandType::AssertReturn: {
      if (IsComponent) {
        break;
      }
      if (!Cmd.Act) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_return has no action";
        break;
      }
      auto Res = RunAction(*Cmd.Act);
      if (!Res) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_return action failed: "
            << WasmEdge::ErrCodeStr[Res.error().getEnum()];
        break;
      }
      EXPECT_TRUE(compareResults(Cmd.Expected, *Res))
          << TestFile << ":" << Cmd.Line << ": assert_return mismatch";
      break;
    }
    case Wast::CommandType::AssertTrap: {
      if (Cmd.Act) {
        if (IsComponent &&
            !checkComponentSupported(UnitName, WasmPhase::Execution)) {
          break;
        }
        if (auto Err = RunFailingAction(Cmd, "assert_trap"sv)) {
          ExpectError(Cmd, *Err, {WasmPhase::Execution}, "assert_trap");
        }
      } else if (!Cmd.ModuleSource.empty() || Cmd.InstanceOperand) {
        if (IsComponent &&
            !checkComponentSupported(UnitName, WasmPhase::Instantiation)) {
          break;
        }
        // The module traps at the instantiation, in its start function or in
        // the initialization of a segment.
        ExpectInstantiationError(Cmd, WasmPhase::Execution, "assert_trap"sv);
      } else {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_trap has no action or module";
      }
      break;
    }
    case Wast::CommandType::AssertExhaustion: {
      if (!Cmd.Act) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_exhaustion has no action";
        break;
      }
      if (IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Execution)) {
        break;
      }
      if (auto Err = RunFailingAction(Cmd, "assert_exhaustion"sv)) {
        EXPECT_EQ(*Err, WasmEdge::ErrCode::Value::CallStackExhausted)
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

      auto [Source, Type] = ResolveSource(Cmd);

      // The converter reports only the syntax errors, and the validator
      // reports the rest. As a result, a text module must parse, and the
      // validator must reject it. The binary loader reports some validation
      // errors itself, so for a binary module the phase of the error code is
      // the test.

      // Run onParse.
      auto ParseRes = ParseSource(Source, Type);
      if (!ParseRes) {
        if (Type == Wast::ModuleType::Text ||
            Type == Wast::ModuleType::TextFile) {
          ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
              << "assert_invalid text module failed to parse: "
              << WasmEdge::ErrCodeStr[ParseRes.error().getEnum()];
        } else {
          ExpectError(Cmd, ParseRes.error(), {WasmPhase::Validation},
                      "assert_invalid");
        }
        break;
      }

      // Run onValidate. This step must fail.
      auto ValRes = onValidate(RootCtx, *ParseRes);
      if (ValRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_invalid expected failure";
      } else {
        ExpectError(Cmd, ValRes.error(), {WasmPhase::Validation},
                    "assert_invalid");
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

      auto [Source, Type] = ResolveSource(Cmd);

      // The phase test is strict for a text module too. This test finds a
      // wrong class of error from the loader or from the validator.

      // Run only onParse. This step must fail.
      auto ParseRes = ParseSource(Source, Type);
      if (ParseRes) {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_malformed expected failure";
      } else {
        ExpectError(Cmd, ParseRes.error(), {WasmPhase::Loading},
                    "assert_malformed");
      }
      break;
    }
    case Wast::CommandType::AssertUnlinkable:
    case Wast::CommandType::AssertUninstantiable: {
      if (IsComponent &&
          !checkComponentSupported(UnitName, WasmPhase::Instantiation)) {
        break;
      }
      // An unlinkable module fails at the import matching or, in WebAssembly
      // 1.0, at the segment bounds. Both come from the instantiation. An
      // uninstantiable module traps, so its error comes from the execution.
      if (Cmd.Type == Wast::CommandType::AssertUnlinkable) {
        ExpectInstantiationError(Cmd, WasmPhase::Instantiation,
                                 "assert_unlinkable"sv);
      } else {
        ExpectInstantiationError(Cmd, WasmPhase::Execution,
                                 "assert_uninstantiable"sv);
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
        if (auto Err = RunFailingAction(Cmd, "assert_exception"sv)) {
          EXPECT_EQ(*Err, WasmEdge::ErrCode::Value::UncaughtException)
              << TestFile << ":" << Cmd.Line
              << ": assert_exception wrong error";
        }
      } else {
        ADD_FAILURE_AT(TestFile.c_str(), Cmd.Line)
            << "assert_exception has no action";
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

      // A shared module keeps its script id in the thread. A register
      // command of the thread then adds the name that its imports use.
      std::vector<std::pair<std::string, std::string>> SharedModules;
      for (const auto &OrigName : Cmd.SharedModules) {
        SharedModules.emplace_back(StoreName(OrigName), SharedName(OrigName));
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
  const bool IsWast = Mode == ParserMode::Wast;
  runFile(TestsuiteRoot / ProposalPath / UName /
              (std::string(UName) + (IsWast ? ".wast"s : ".json"s)),
          Conf, isComponentModel(ProposalPath));
}

void SpecTest::runFile(const std::filesystem::path &Path, const Configure &Conf,
                       bool IsComp) {
  auto RootCtx = onInit(nullptr, {});
  const bool IsWast = Mode == ParserMode::Wast;

  // A script that does not parse is a failure, and not a skip. A skip hides
  // a corrupt test unit behind a green result.
  const auto Script =
      IsWast ? Wast::parseWast(Path) : Wast::parseJson(Path, !IsComp);
  if (Script && Script->Commands.empty()) {
    ADD_FAILURE_AT(u8string(Path).c_str(), 1)
        << "The script has no command: " << u8string(Path);
  } else if (Script) {
    executeCommands(Script->Commands, Conf, IsComp, RootCtx, u8string(Path));
  } else {
    ADD_FAILURE_AT(u8string(Path).c_str(), 1)
        << "Failed to parse " << (IsWast ? "WAST" : "JSON") << ": "
        << u8string(Path);
  }
  onFini(RootCtx);
}

} // namespace WasmEdge

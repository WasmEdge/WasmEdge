// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/gc/GCCapabilityTest.cpp - GC capability tests -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests for the GC capability of compiled code:
/// the safepoint poll in the prologue, the instantiate and per-call
/// capability gates, and the capability bit in AOT artifacts.
///
//===----------------------------------------------------------------------===//

#include "gchelpers.h"

#include "aot/version.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iterator>

namespace {

using namespace WasmEdge;
using namespace WasmEdge::GCTest;

#ifdef WASMEDGE_USE_LLVM

// --- Executor-enforced GC capability gate ---
//
// A GC-off compilation emits no cooperative safepoint poll and no shadow-root
// spill, so its native code is unsafe under a concurrent collector: it would
// never yield at a stop-the-world and would publish no native roots. The
// capability is recorded per module (AST::Module::getGCCompiled) and the
// Executor::instantiate function gate refuses to bind a non-capable module's
// compiled symbols under a GC-enabled executor, deopting the whole module to
// interpreter execution (which is cooperatively safe). This drives the gate's
// full decision matrix by compiling one ref-using module GC-off (via the manual
// Loader/Compiler/JIT path, so compile config and execute config differ) and
// instantiating it under executors with/without the GC proposal.

// (module (func (export "run") (result i32) (local funcref) i32.const 42))
const std::vector<uint8_t> RefLocalWasm = {
    0, 97, 115, 109, 1,   0,   0, 0, 1,  5, 1, 96, 0, 1, 127, 3,  2,  1, 0, 7,
    7, 1,  3,   114, 117, 110, 0, 0, 10, 8, 1, 6,  1, 1, 112, 65, 42, 11};

// Manual parse -> validate -> compile -> JIT -> attach symbols under a chosen
// Configure, so the artifact's compile-time GC capability is independent of the
// executor that later runs it. Returns the module (carrying compiled symbols)
// or nullptr on any failure.
std::shared_ptr<AST::Module> compileWithConfig(const Configure &Conf,
                                               Span<const uint8_t> Bytes) {
  Loader::Loader LoaderEngine(Conf, &Executor::Executor::Intrinsics);
  Validator::Validator ValidatorEngine(Conf);
  auto ModOrErr = LoaderEngine.parseModule(Bytes);
  if (!ModOrErr) {
    return nullptr;
  }
  std::shared_ptr<AST::Module> Mod{std::move(*ModOrErr)};
  if (!ValidatorEngine.validate(*Mod)) {
    return nullptr;
  }
  LLVM::Compiler Compiler(Conf);
  if (!Compiler.checkConfigure()) {
    return nullptr;
  }
  auto Data = Compiler.compile(*Mod);
  if (!Data) {
    return nullptr;
  }
  LLVM::JIT JIT(Conf);
  auto Exec = JIT.load(std::move(*Data));
  if (!Exec) {
    return nullptr;
  }
  if (!LoaderEngine.loadExecutable(*Mod, std::move(*Exec))) {
    return nullptr;
  }
  return Mod;
}

// (module (func (export "f") (param i32) (result i32)
//   (i32.add (local.get 0) (i32.const 1))))
const std::vector<uint8_t> LoopFreeWasm = {
    0, 97, 115, 109, 1, 0,   0, 0, 1,  6, 1, 96, 1, 127, 1, 127, 3, 2,   1,
    0, 7,  5,   1,   1, 102, 0, 0, 10, 9, 1, 7,  0, 32,  0, 65,  1, 106, 11};

// Count the safepoint parks ("gcsp.park" blocks) the compiler emitted for
// LoopFreeWasm under Conf, by compiling it with the IR dump enabled and
// reading the JIT's dump (wasm-jit.ll in the working directory).
size_t countCompiledSafepointPolls(Configure Conf) {
  Conf.getCompilerConfigure().setDumpIR(true);
  std::remove("wasm-jit.ll");
  auto Mod = compileWithConfig(Conf, LoopFreeWasm);
  if (Mod == nullptr) {
    return SIZE_MAX;
  }
  std::ifstream In("wasm-jit.ll");
  std::string IR((std::istreambuf_iterator<char>(In)),
                 std::istreambuf_iterator<char>());
  std::remove("wasm-jit.ll");
  size_t N = 0;
  for (size_t Pos = IR.find("gcsp.park"); Pos != std::string::npos;
       Pos = IR.find("gcsp.park", Pos + 1)) {
    ++N;
  }
  return N;
}

// The safepoint poll at loop back-edges bounds a stop-the-world only for code
// that loops. A loop-free compiled function that recurses deeply (a recursive
// walk over a large structure via direct calls, no loop headers) would keep a
// collection's STW -- and every mutator already parked for it -- waiting until
// it unwound to the executor. The interpreter polls on every function entry;
// compiled code must poll in its prologue too. Asserted structurally on the
// emitted IR: a loop-free function carries at least one poll under GC, and
// none when the GC proposal is off (the poll stays gated on the proposal).
TEST(GCThread, CompiledFunctionPrologueHasSafepointPoll) {
  SKIP_WITHOUT_COMPILER();
  Configure GCConf = makeGCConf();
  GCConf.getRuntimeConfigure().setRunMode(RunMode::JIT);
  const size_t WithGC = countCompiledSafepointPolls(GCConf);
  ASSERT_NE(WithGC, SIZE_MAX);
  EXPECT_GE(WithGC, 1u) << "a loop-free compiled function must poll the GC "
                           "safepoint in its prologue";

  Configure PlainConf;
  PlainConf.removeProposal(Proposal::GC);
  PlainConf.getRuntimeConfigure().setRunMode(RunMode::JIT);
  const size_t WithoutGC = countCompiledSafepointPolls(PlainConf);
  ASSERT_NE(WithoutGC, SIZE_MAX);
  EXPECT_EQ(WithoutGC, 0u) << "no poll without the GC proposal";
}

// Compile the ref-using module GC-off (a non-capable artifact: symbols present,
// no safepoint poll / shadow spill), stamp its capability to GCCapable, then
// instantiate under an executor whose GC proposal is ExecutorGC. Returns
// whether "run" bound to native compiled code. Each call uses a fresh module so
// compiled binding is independent across scenarios.
bool runIsCompiled(bool GCCapable, bool ExecutorGC) {
  // Configure() enables the full standard proposal set including GC, so a
  // GC-off compile/executor is produced by removeProposal(GC), not by building
  // up from empty. Compiling with GC removed yields a genuinely non-capable
  // artifact (no safepoint poll / shadow spill emitted).
  Configure CompileConf;
  if (!GCCapable) {
    CompileConf.removeProposal(Proposal::GC);
  }
  auto Mod = compileWithConfig(CompileConf, RefLocalWasm);
  EXPECT_NE(Mod, nullptr);
  if (!Mod) {
    return false;
  }
  // On the VM path vm.cpp stamps this from the compile config; the manual path
  // sets it explicitly to match the GC-off vs GC-on compilation above.
  Mod->setGCCompiled(GCCapable);

  Configure ExecConf;
  if (!ExecutorGC) {
    ExecConf.removeProposal(Proposal::GC);
  }
  Executor::Executor Exec(ExecConf);
  Runtime::StoreManager Store;
  auto InstOrErr = Exec.instantiateModule(Store, *Mod);
  EXPECT_TRUE(InstOrErr);
  if (!InstOrErr) {
    return false;
  }
  auto Inst = std::move(*InstOrErr);
  const auto *RunFn = Inst->findFuncExports("run");
  EXPECT_NE(RunFn, nullptr);
  return RunFn != nullptr && RunFn->isCompiledFunction();
}

TEST(GCThread, CapabilityGateDeoptsNonCapableModuleUnderGCExecutor) {
  // The gate fires in exactly one quadrant of (module capability x executor
  // GC): a non-capable module under a GC-enabled executor deopts to the
  // interpreter; every other combination runs the compiled code natively.
  //
  // Baseline / capable + non-GC: proves the module compiles and native binding
  // is observable.
  EXPECT_TRUE(runIsCompiled(/*GCCapable=*/true, /*ExecutorGC=*/false));
  // The gate: non-capable + GC-enabled -> deopt to interpreter.
  EXPECT_FALSE(runIsCompiled(/*GCCapable=*/false, /*ExecutorGC=*/true));
  // Negative control: the deopt is gated on the executor's GC proposal, so a
  // non-capable module under a non-GC executor still runs native (no collector
  // to endanger it).
  EXPECT_TRUE(runIsCompiled(/*GCCapable=*/false, /*ExecutorGC=*/false));
  // Negative control: the deopt is gated on the module's capability, so a
  // capable module under a GC executor runs native.
  EXPECT_TRUE(runIsCompiled(/*GCCapable=*/true, /*ExecutorGC=*/true));

  // End-to-end: the deopted (interpreter) function is genuinely runnable and
  // returns the right value -- the fallback is real execution, not a stub.
  Configure CompileConf; // full set minus GC -> non-capable artifact
  CompileConf.removeProposal(Proposal::GC);
  auto Mod = compileWithConfig(CompileConf, RefLocalWasm);
  ASSERT_NE(Mod, nullptr);
  Mod->setGCCompiled(false); // non-capable
  Configure GCConf;          // default: GC enabled
  Executor::Executor Exec(GCConf);
  Runtime::StoreManager Store;
  auto InstOrErr = Exec.instantiateModule(Store, *Mod);
  ASSERT_TRUE(InstOrErr);
  auto Inst = std::move(*InstOrErr);
  const auto *RunFn = Inst->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  EXPECT_FALSE(RunFn->isCompiledFunction());
  auto R = Exec.invoke(RunFn, {}, {});
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), UINT32_C(42));
}

// Instantiate Wasm compiled under CompileGC using an executor whose GC proposal
// is InstantiateGC, then invoke the resulting function instance through a
// second executor whose GC proposal is InvokeGC. This is the shape the
// instantiate gate cannot see: the module never passes through the invoking
// executor's instantiate, so only a per-call check can catch it. Returns the
// invocation result.
Expect<std::vector<std::pair<ValVariant, ValType>>>
invokeAcrossExecutors(bool CompileGC, bool InstantiateGC, bool InvokeGC,
                      bool *OutWasCompiled) {
  Configure CompileConf;
  if (!CompileGC) {
    CompileConf.removeProposal(Proposal::GC);
  }
  auto Mod = compileWithConfig(CompileConf, RefLocalWasm);
  EXPECT_NE(Mod, nullptr);
  Mod->setGCCompiled(CompileGC);

  Configure InstConf;
  if (!InstantiateGC) {
    InstConf.removeProposal(Proposal::GC);
  }
  Executor::Executor OwnerExec(InstConf);
  Runtime::StoreManager OwnerStore;
  auto InstOrErr = OwnerExec.instantiateModule(OwnerStore, *Mod);
  EXPECT_TRUE(InstOrErr);
  auto Inst = std::move(*InstOrErr);
  const auto *RunFn = Inst->findFuncExports("run");
  EXPECT_NE(RunFn, nullptr);
  if (OutWasCompiled != nullptr) {
    *OutWasCompiled = RunFn->isCompiledFunction();
  }

  Configure InvokeConf;
  if (!InvokeGC) {
    InvokeConf.removeProposal(Proposal::GC);
  }
  Executor::Executor Foreign(InvokeConf);
  return Foreign.invoke(RunFn, {}, {});
}

// Compiled code has one more way into another module: the call_ref and
// call_indirect slow paths ask the runtime for the callee's symbol and then
// call it directly, never passing enterFunction. That hand-off must apply the
// same ownership check, or a compiled caller reaches a foreign-owned module's
// compiled code with no check at all.
TEST(GCThread, CompiledCalleeModuleRootsCheckedPerCall) {
  Configure Conf = makeGCConf();
  auto NMod = compileWithConfig(Conf, ForeignRootsWasm);
  ASSERT_NE(NMod, nullptr);
  auto MMod = compileWithConfig(Conf, CalleeImportWasm);
  ASSERT_NE(MMod, nullptr);

  Executor::Executor Owner(Conf);
  Runtime::StoreManager OwnerStore;
  auto NInstOrErr = Owner.registerModule(OwnerStore, *NMod, "N");
  ASSERT_TRUE(NInstOrErr);
  auto NInst = std::move(*NInstOrErr);
  auto MInstOrErr = Owner.instantiateModule(OwnerStore, *MMod);
  ASSERT_TRUE(MInstOrErr);
  auto MInst = std::move(*MInstOrErr);
  // Guard against a vacuous pass: both sides must really be native, or the
  // interpreter path (already covered above) is what gets exercised.
  ASSERT_TRUE(NInst->findFuncExports("set")->isCompiledFunction());
  ASSERT_TRUE(MInst->findFuncExports("run_ref")->isCompiledFunction());
  const auto *GetFn = NInst->findFuncExports("get");
  ASSERT_NE(GetFn, nullptr);

  Executor::Executor Foreign(Conf);
  for (const char *Entry : {"run", "run_ref", "run_indirect"}) {
    const auto *Fn = MInst->findFuncExports(Entry);
    ASSERT_NE(Fn, nullptr) << Entry;
    auto Refused = Foreign.invoke(Fn, {}, {});
    ASSERT_FALSE(Refused) << Entry << " ran code of a foreign-owned module";
    EXPECT_EQ(Refused.error(), ErrCode::Value::IncompatibleImportType) << Entry;
    EXPECT_EQ(NInst->getGCRootsOwnerId(), Owner.getAllocator().getId());
  }

  for (const char *Entry : {"run", "run_ref", "run_indirect"}) {
    ASSERT_TRUE(Owner.invoke(MInst->findFuncExports(Entry), {}, {})) << Entry;
    auto Got = Owner.invoke(GetFn, {}, {});
    ASSERT_TRUE(Got) << Entry;
    ASSERT_EQ(Got->size(), 1u);
    EXPECT_EQ((*Got)[0].first.get<uint32_t>(), UINT32_C(7)) << Entry;
  }
}

// (module (func (export "set")))
const std::vector<uint8_t> PlainSetWasm = {
    0, 97, 115, 109, 1, 0,   0,   0,   1, 4, 1,  96, 0, 0, 3, 2, 1,
    0, 7,  7,   1,   3, 115, 101, 116, 0, 0, 10, 4,  1, 2, 0, 11};

// The same hand-off is the one place the per-call GC capability backstop did
// not reach: enterFunction refuses compiled code of a module built without GC
// support, but a direct symbol call never gets there. A GC executor must not
// be handed a non-capable module's symbol; the hand-off declines and the
// proxied call refuses as it does for a plain invoke.
TEST(GCThread, CompiledCalleeCapabilityCheckedOnDirectHandoff) {
  Configure NoGC;
  NoGC.removeProposal(Proposal::GC);
  auto NMod = compileWithConfig(NoGC, PlainSetWasm);
  ASSERT_NE(NMod, nullptr);
  NMod->setGCCompiled(false);
  Configure Conf = makeGCConf();
  auto MMod = compileWithConfig(Conf, CalleeImportWasm);
  ASSERT_NE(MMod, nullptr);

  // N is bound to native code by a GC-off executor (so the instantiate deopt
  // never fires) and published without a GC claim; M is a GC executor's own.
  Executor::Executor Plain(NoGC);
  Runtime::StoreManager Store;
  auto NInstOrErr = Plain.registerModule(Store, *NMod, "N");
  ASSERT_TRUE(NInstOrErr);
  auto NInst = std::move(*NInstOrErr);
  ASSERT_TRUE(NInst->findFuncExports("set")->isCompiledFunction());
  Executor::Executor Exec(Conf);
  auto MInstOrErr = Exec.instantiateModule(Store, *MMod);
  ASSERT_TRUE(MInstOrErr);
  auto MInst = std::move(*MInstOrErr);
  ASSERT_TRUE(MInst->findFuncExports("run_ref")->isCompiledFunction());

  for (const char *Entry : {"run", "run_ref", "run_indirect"}) {
    auto Refused = Exec.invoke(MInst->findFuncExports(Entry), {}, {});
    ASSERT_FALSE(Refused) << Entry << " ran non-GC native code";
    EXPECT_EQ(Refused.error(), ErrCode::Value::IllegalGrammar) << Entry;
  }
  // Control: the GC-off executor runs the same paths.
  for (const char *Entry : {"run", "run_ref", "run_indirect"}) {
    EXPECT_TRUE(Plain.invoke(MInst->findFuncExports(Entry), {}, {})) << Entry;
  }
}

TEST(GCThread, PerCallCapabilityCheckRefusesForeignNonCapableModule) {
  // The instantiate gate is a choke point for modules an executor instantiates
  // itself. registerModule and a direct Executor::invoke both bypass it: a
  // GC-off executor binds compiled code with no safepoint poll and no shadow
  // spill, and a GC-enabled executor can then be handed those very function
  // instances. enterFunction is the single funnel every call passes through, so
  // the backstop lives there.

  // The backstop: compiled GC-off, instantiated by a GC-off executor (so the
  // instantiate deopt never fires and the code stays native), then invoked
  // through a GC-enabled executor.
  bool WasCompiled = false;
  auto Refused = invokeAcrossExecutors(
      /*CompileGC=*/false, /*InstantiateGC=*/false, /*InvokeGC=*/true,
      &WasCompiled);
  // Guard against a vacuous pass: if the owning executor had deopted this to
  // the interpreter there would be no native code to refuse, and the assertion
  // below would prove nothing.
  ASSERT_TRUE(WasCompiled);
  ASSERT_FALSE(Refused);
  EXPECT_EQ(Refused.error(), ErrCode::Value::IllegalGrammar);

  // Control: the refusal is the invoking executor's GC proposal, not something
  // broken about a cross-executor invocation. The same non-capable instance
  // called through a GC-off executor runs its compiled code and returns 42.
  auto Allowed = invokeAcrossExecutors(
      /*CompileGC=*/false, /*InstantiateGC=*/false, /*InvokeGC=*/false,
      nullptr);
  ASSERT_TRUE(Allowed);
  ASSERT_EQ(Allowed->size(), 1u);
  EXPECT_EQ((*Allowed)[0].first.get<uint32_t>(), UINT32_C(42));

  // Control: the refusal is keyed on capability, not on the module being
  // foreign. A GC-capable module instantiated elsewhere still runs compiled
  // under a GC-enabled executor.
  bool CapableWasCompiled = false;
  auto Capable = invokeAcrossExecutors(
      /*CompileGC=*/true, /*InstantiateGC=*/false, /*InvokeGC=*/true,
      &CapableWasCompiled);
  ASSERT_TRUE(CapableWasCompiled);
  ASSERT_TRUE(Capable);
  ASSERT_EQ(Capable->size(), 1u);
  EXPECT_EQ((*Capable)[0].first.get<uint32_t>(), UINT32_C(42));
}

// --- Durable AOT capability bit + loader admission ---
//
// The gate is only as strong as the capability that reaches it. An artifact
// compiled in one process and loaded in another carries no compile Configure,
// so the capability has to survive inside the artifact. The compiler exports a
// "gc.capable" marker global only when GC codegen is on; for a universal WASM
// outputWasmLibrary records the marker's presence as a flag byte in the AOT
// section, which the loader reads back into AST::Module::getGCCompiled.
//
// Admission has to happen in the loader, not at instantiate: loading in AOT run
// mode skips parsing function bodies (Loader::loadSegment), so a module bound
// to non-capable native code has no instructions left to deopt to. The loader
// instead declines the bind and takes its interpreter fallback, which re-reads
// the skipped code section.

// AOT-compile the ref-using module to a universal WASM at Out with the GC
// proposal set to GCCapable, then load it back in AOT run mode with the GC
// proposal set to LoadWithGC. The reloaded module carries whatever capability
// the artifact itself recorded. If PatchArtifact is set, it can change the file
// between the write and the reload. Returns nullptr on any failure.
std::unique_ptr<AST::Module>
aotRoundTrip(bool GCCapable, bool LoadWithGC, const std::filesystem::path &Out,
             const std::function<bool(const std::filesystem::path &)>
                 &PatchArtifact = {}) {
  Configure CompileConf;
  if (!GCCapable) {
    CompileConf.removeProposal(Proposal::GC);
  }
  Loader::Loader LoaderEngine(CompileConf, &Executor::Executor::Intrinsics);
  Validator::Validator ValidatorEngine(CompileConf);
  auto ModOrErr = LoaderEngine.parseModule(RefLocalWasm);
  if (!ModOrErr) {
    return nullptr;
  }
  if (!ValidatorEngine.validate(**ModOrErr)) {
    return nullptr;
  }
  LLVM::Compiler Compiler(CompileConf);
  if (!Compiler.checkConfigure()) {
    return nullptr;
  }
  auto Data = Compiler.compile(**ModOrErr);
  if (!Data) {
    return nullptr;
  }
  // Default OutputFormat is Wasm, so this emits a universal WASM carrying the
  // AOT section -- the path that serializes the capability flag byte.
  LLVM::CodeGen CodeGen(CompileConf);
  if (!CodeGen.codegen(RefLocalWasm, std::move(*Data), Out)) {
    return nullptr;
  }
  if (PatchArtifact && !PatchArtifact(Out)) {
    return nullptr;
  }

  Configure LoadConf;
  if (!LoadWithGC) {
    LoadConf.removeProposal(Proposal::GC);
  }
  LoadConf.getRuntimeConfigure().setRunMode(RunMode::AOT);
  Loader::Loader Reloader(LoadConf, &Executor::Executor::Intrinsics);
  auto Reloaded = Reloader.parseModule(Out);
  if (!Reloaded) {
    return nullptr;
  }
  // A freshly loaded module has not passed validation, which instantiate
  // requires. Validating does not touch the attached symbols or the capability.
  Validator::Validator Revalidator(LoadConf);
  if (!Revalidator.validate(**Reloaded)) {
    return nullptr;
  }
  return std::move(*Reloaded);
}

// Instantiate under a GC-enabled executor and return whether "run" bound to
// native code; also asserts the function actually computes 42, so a deopt to an
// empty (body-stripped) function instance cannot pass as a successful fallback.
bool aotRunIsCompiled(AST::Module &Mod) {
  Configure GCConf; // default: GC enabled
  Executor::Executor Exec(GCConf);
  Runtime::StoreManager Store;
  auto InstOrErr = Exec.instantiateModule(Store, Mod);
  EXPECT_TRUE(InstOrErr);
  if (!InstOrErr) {
    return false;
  }
  auto Inst = std::move(*InstOrErr);
  const auto *RunFn = Inst->findFuncExports("run");
  EXPECT_NE(RunFn, nullptr);
  if (RunFn == nullptr) {
    return false;
  }
  auto R = Exec.invoke(RunFn, {}, {});
  EXPECT_TRUE(R);
  if (!R) {
    return false;
  }
  EXPECT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), UINT32_C(42));
  return RunFn->isCompiledFunction();
}

TEST(GCThread, DurableAOTCapabilityBitSurvivesRoundTrip) {
  const auto Dir = std::filesystem::current_path();
  const auto CapablePath = Dir / "gc-capable.wasm";
  const auto OffPath = Dir / "gc-off.wasm";
  std::error_code EC;

  // Load with GC off so the loader always binds the native code: this isolates
  // "did the bit survive the round trip" from "what the loader does about it".
  {
    auto Capable =
        aotRoundTrip(/*GCCapable=*/true, /*LoadWithGC=*/false, CapablePath);
    ASSERT_NE(Capable, nullptr);
    // getSymbol() confirms the AOT section really linked -- otherwise the
    // loader silently fell back and the capability below would be vacuous.
    EXPECT_TRUE(!!Capable->getSymbol());
    EXPECT_TRUE(Capable->getGCCompiled());

    auto NonCapable =
        aotRoundTrip(/*GCCapable=*/false, /*LoadWithGC=*/false, OffPath);
    ASSERT_NE(NonCapable, nullptr);
    EXPECT_TRUE(!!NonCapable->getSymbol());
    // The durable bit: compiled GC-off in an earlier step, written to disk, and
    // still correctly reported as non-capable after a reload.
    EXPECT_FALSE(NonCapable->getGCCompiled());
  }

  // Load with GC on: the loader must admit the capable artifact and decline the
  // non-capable one, and both must still execute correctly.
  {
    auto Capable =
        aotRoundTrip(/*GCCapable=*/true, /*LoadWithGC=*/true, CapablePath);
    ASSERT_NE(Capable, nullptr);
    EXPECT_TRUE(!!Capable->getSymbol());
    EXPECT_TRUE(aotRunIsCompiled(*Capable));

    auto NonCapable =
        aotRoundTrip(/*GCCapable=*/false, /*LoadWithGC=*/true, OffPath);
    ASSERT_NE(NonCapable, nullptr);
    // Admission: the loader refused to bind non-capable native code, so no
    // symbols are attached and the skipped function bodies were re-read.
    EXPECT_FALSE(!!NonCapable->getSymbol());
    // Runs interpreted -- and aotRunIsCompiled asserts it returns 42, which
    // is what proves the bodies came back rather than being empty.
    EXPECT_FALSE(aotRunIsCompiled(*NonCapable));
  }

  std::filesystem::remove(CapablePath, EC);
  std::filesystem::remove(OffPath, EC);
}

// Set the AOT binary version in the "wasmedge" custom section of the universal
// WASM at Path to Version. The version directly follows the section name, and
// both the current and the patched value fit in one LEB128 byte.
bool patchAOTBinaryVersion(const std::filesystem::path &Path, uint8_t Version) {
  std::vector<char> Bytes;
  {
    std::ifstream In(Path, std::ios::binary);
    Bytes.assign(std::istreambuf_iterator<char>(In),
                 std::istreambuf_iterator<char>());
  }
  const std::string Name = "\x08wasmedge";
  auto It = std::search(Bytes.begin(), Bytes.end(), Name.begin(), Name.end());
  EXPECT_NE(It, Bytes.end());
  if (It == Bytes.end()) {
    return false;
  }
  auto VersionIt = It + static_cast<std::ptrdiff_t>(Name.size());
  if (VersionIt == Bytes.end()) {
    return false;
  }
  EXPECT_EQ(static_cast<uint8_t>(*VersionIt), AOT::kBinaryVersion);
  *VersionIt = static_cast<char>(Version);
  std::ofstream Out(Path, std::ios::binary | std::ios::trunc);
  Out.write(Bytes.data(), static_cast<std::streamsize>(Bytes.size()));
  return static_cast<bool>(Out);
}

// AOT binary version 3 shipped in 0.18.0-alpha.1. Its AOT section has no GC
// capability byte and its code uses the older ExecCtx and intrinsics layout,
// so the loader must not bind native code that claims that version. It must
// fall back to the interpreter instead.
TEST(GCThread, AOTArtifactWithReleasedVersion3IsNotBound) {
  const auto Path = std::filesystem::current_path() / "gc-aot-v3.wasm";
  std::error_code EC;
  // Load with GC off so that only the version check can decline the bind.
  auto Mod = aotRoundTrip(
      /*GCCapable=*/true, /*LoadWithGC=*/false, Path,
      [](const std::filesystem::path &P) {
        return patchAOTBinaryVersion(P, 3);
      });
  ASSERT_NE(Mod, nullptr);
  EXPECT_FALSE(!!Mod->getSymbol());
  std::filesystem::remove(Path, EC);
}

// --- Component core-module capability hole ---
//
// The component core-module instantiate path (component_module.cpp) used to
// call the function-section instantiate() without the capability argument,
// so it always took the `bool GCCompiled = true` default: DeoptForGC was
// always false and native symbols always bound, regardless of whether the
// core module was actually compiled GC-capable. The freshly built
// ModuleInstance was never setGCCompiled()-stamped either, so the per-call
// enterFunction backstop (helper.cpp) could not catch it after the fact.
// This test hand-assembles a minimal component AST embedding a GC-off
// compiled core module (mirroring the manual Loader/Compiler/JIT path used
// above) and instantiates it under a GC-enabled executor, matching the
// non-component gate exercised by CapabilityGateDeoptsNonCapableModuleUnder
// GCExecutor.
TEST(GCThread, ComponentCoreModuleCapabilityGated) {
  // Compile the ref-using core module GC-off: symbols present, but no
  // safepoint poll / shadow-root spill emitted.
  Configure CompileConf;
  CompileConf.removeProposal(Proposal::GC);
  auto Mod = compileWithConfig(CompileConf, RefLocalWasm);
  ASSERT_NE(Mod, nullptr);
  Mod->setGCCompiled(false);

  // Hand-assemble a component: a CoreModuleSection carrying the compiled
  // module, and a CoreInstanceSection that instantiates it with no imports.
  // This is the shape the production loader builds from a `.wasm` component
  // binary's `core:module` + `core:instance (instantiate ...)` sections; it
  // is built in-process here because there is no on-disk component fixture
  // wired to a compiled (as opposed to interpreted) core module.
  AST::Component::Component Comp;
  {
    AST::Component::CoreModuleSection ModSec;
    ModSec.getContent() = std::move(*Mod);
    Comp.getSections().emplace_back(std::move(ModSec));
  }
  {
    AST::Component::CoreInstanceSection InstSec;
    AST::Component::CoreInstance CoreInst;
    CoreInst.setInstantiateArgs(/*ModIdx=*/0, {});
    InstSec.getContent().push_back(std::move(CoreInst));
    Comp.getSections().emplace_back(std::move(InstSec));
  }

  Configure GCConf; // default: GC enabled
  Executor::Executor Exec(GCConf);
  Runtime::StoreManager Store;
  auto CompInstOrErr = Exec.instantiateComponent(Store, Comp);
  ASSERT_TRUE(CompInstOrErr);
  auto CompInst = std::move(*CompInstOrErr);

  const auto *CoreModInst = CompInst->getCoreModuleInstance(0);
  ASSERT_NE(CoreModInst, nullptr);
  // The backstop stamp: the freshly built component core-module instance
  // carries the same non-capability as the AST module it was built from, so
  // enterFunction covers it even for calls that bypass this instantiate.
  EXPECT_FALSE(CoreModInst->isGCCompiled());

  const auto *RunFn = CoreModInst->findFuncExports("run");
  ASSERT_NE(RunFn, nullptr);
  // The gate: a non-capable core module under a GC-enabled executor must not
  // bind native compiled code.
  EXPECT_FALSE(RunFn->isCompiledFunction());

  // The deopted (interpreter) function is genuinely runnable and returns the
  // right value -- the fallback is real execution, not a stub.
  auto R = Exec.invoke(RunFn, {}, {});
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 1u);
  EXPECT_EQ((*R)[0].first.get<uint32_t>(), UINT32_C(42));

  // Negative control: the deopt is gated on the executor's GC proposal, not
  // unconditional. The same GC-off-compiled core module, embedded in a fresh
  // component and instantiated under a GC-disabled executor, still runs its
  // compiled code natively -- there is no collector to endanger it, so
  // nothing should be refused.
  auto Mod2 = compileWithConfig(CompileConf, RefLocalWasm);
  ASSERT_NE(Mod2, nullptr);
  Mod2->setGCCompiled(false);

  AST::Component::Component Comp2;
  {
    AST::Component::CoreModuleSection ModSec;
    ModSec.getContent() = std::move(*Mod2);
    Comp2.getSections().emplace_back(std::move(ModSec));
  }
  {
    AST::Component::CoreInstanceSection InstSec;
    AST::Component::CoreInstance CoreInst;
    CoreInst.setInstantiateArgs(/*ModIdx=*/0, {});
    InstSec.getContent().push_back(std::move(CoreInst));
    Comp2.getSections().emplace_back(std::move(InstSec));
  }

  Configure NonGCConf;
  NonGCConf.removeProposal(Proposal::GC);
  Executor::Executor NonGCExec(NonGCConf);
  Runtime::StoreManager Store2;
  auto CompInstOrErr2 = NonGCExec.instantiateComponent(Store2, Comp2);
  ASSERT_TRUE(CompInstOrErr2);
  auto CompInst2 = std::move(*CompInstOrErr2);
  const auto *CoreModInst2 = CompInst2->getCoreModuleInstance(0);
  ASSERT_NE(CoreModInst2, nullptr);
  const auto *RunFn2 = CoreModInst2->findFuncExports("run");
  ASSERT_NE(RunFn2, nullptr);
  EXPECT_TRUE(RunFn2->isCompiledFunction());
  auto R2 = NonGCExec.invoke(RunFn2, {}, {});
  ASSERT_TRUE(R2);
  ASSERT_EQ(R2->size(), 1u);
  EXPECT_EQ((*R2)[0].first.get<uint32_t>(), UINT32_C(42));
}

#endif // WASMEDGE_USE_LLVM

} // namespace

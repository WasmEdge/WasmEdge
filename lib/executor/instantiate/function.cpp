// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include <cstdint>
#include <utility>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

// Instantiate function instance. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::FunctionSection &FuncSec,
                                   const AST::CodeSection &CodeSec,
                                   bool GCCompiled) {

  // Get the function type indices.
  auto TypeIdxs = FuncSec.getContent();
  auto CodeSegs = CodeSec.getContent();

  if (CodeSegs.size() == 0) {
    return {};
  }
  // R7-M3 capability gate: a GC-enabled executor must not run a module whose
  // compiled code lacks GC support (no cooperative safepoint poll, no
  // shadow-root spill) natively -- a concurrent collection would hang on it or
  // miss its roots. When the executor has the GC proposal enabled and the
  // compiled code is not GC-capable, deopt to interpreter execution for the
  // whole module by taking the `else` branch below, which builds function
  // instances from the locals + instructions. This is the single choke point
  // all instantiations funnel through, and the backstop for paths where the
  // loader could not decide (its Configure may differ from this executor's).
  const bool DeoptForGC = Conf.hasProposal(Proposal::GC) && !GCCompiled;
  if (DeoptForGC && CodeSegs[0].getExpr().getInstrs().empty()) {
    // The interpreter fallback needs instructions, but loading an artifact in
    // AOT run mode SKIPS parsing function bodies (see Loader::loadSegment), so
    // there is nothing to deopt to. Refuse loudly instead of building empty
    // function instances, which would silently return garbage. The loader
    // declines such a bind up front (Loader::loadUniversalWASM and the shared
    // library path in Loader::parseWasmUnit), so reaching here means the module
    // was loaded under a Configure whose GC proposal differed from this
    // executor's -- reload it with a matching configuration.
    spdlog::error("Module was AOT-compiled without GC support and its function "
                  "bodies were stripped at load time, so it can neither run "
                  "natively under a GC-enabled executor nor fall back to the "
                  "interpreter. Reload it with a matching configuration."sv);
    return Unexpect(ErrCode::Value::IllegalGrammar);
  }
  // Under interpreter mode, the module always chooses the `for` loop in the
  // `else` case. Branching in the `for` loop might cause meaningless branch
  // misses, so check the first item and dispatch it into different cases to
  // reduce branch misses.
  if (CodeSegs[0].getSymbol() != false && !DeoptForGC) {
    for (uint32_t I = 0; I < CodeSegs.size(); ++I) {
      auto Symbol = CodeSegs[I].getSymbol();
      ModInst.addFunc(
          TypeIdxs[I],
          (*ModInst.getType(TypeIdxs[I]))->getCompositeType().getFuncType(),
          std::move(Symbol));
    }
  } else {
    // Iterate through the code segments to instantiate function instances.
    for (uint32_t I = 0; I < CodeSegs.size(); ++I) {
      // Create and add the function instance to the module instance.
      ModInst.addFunc(
          TypeIdxs[I],
          (*ModInst.getType(TypeIdxs[I]))->getCompositeType().getFuncType(),
          CodeSegs[I].getLocals(), CodeSegs[I].getExpr().getInstrs());
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge

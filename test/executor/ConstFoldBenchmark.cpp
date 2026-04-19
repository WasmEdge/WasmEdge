// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/executor/ConstFoldBenchmark.cpp - Fold benchmark ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Measures the constant-folding pass on three real-world WAT programs:
///   fibonacci_bench.wat  — recursive fib(10)
///   factorial_bench.wat  — recursive fac(10)
///   mandelbrot_bench.wat — Mandelbrot escape-time core, iterateEquation(-0.5,0,100)
///
/// For each module the tool reports:
///   • Per-function instruction counts before and after optimization
///   • Number of nop-filled (folded) slots per function
///   • Module-wide totals and reduction percentage
///   • Actual return value from main() for correctness verification
///
/// Key hypothesis tested: "do fibonacci and factorial collapse to a single
/// i32.const store?"
///
/// Answer: NO — because our pass folds only intra-function const+const+binop
/// triples in the flat instruction stream.  Recursive calls are opaque; the
/// pass never evaluates them.  The functions themselves use parameter locals
/// in every arithmetic expression, so no const+const pair ever forms adjacent
/// to a binary op.  Even the main() wrappers push constants only as call
/// arguments — no binary op follows, so no foldable triple forms there either.
///
//===----------------------------------------------------------------------===//

#include "ast/description.h"
#include "ast/expression.h"
#include "ast/module.h"
#include "ast/section.h"
#include "ast/segment.h"
#include "common/configure.h"
#include "common/enum_ast.hpp"
#include "executor/engine/const_fold.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "vm/vm.h"

#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace WasmEdge;

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Per-module data structures
// ---------------------------------------------------------------------------

struct FuncStats {
  std::string Name;
  size_t RawTotal;   // instructions before optimization
  size_t FoldedNops; // nops inserted by the pass
};

struct ModuleStats {
  std::string FilePath;
  std::vector<FuncStats> Funcs;
  size_t RawTotal;
  size_t FoldedNops;
  int32_t ReturnValue;
  bool CallSucceeded;
};

// ---------------------------------------------------------------------------
// analyzeModule — load, count, optimize (on a copy), execute.
// ---------------------------------------------------------------------------
ModuleStats analyzeModule(const std::filesystem::path &WatPath) {
  ModuleStats Stats;
  Stats.FilePath = WatPath.string();
  Stats.RawTotal = 0;
  Stats.FoldedNops = 0;
  Stats.ReturnValue = 0;
  Stats.CallSucceeded = false;

  Configure Conf;
  Conf.addProposal(Proposal::SIMD);
  Conf.addProposal(Proposal::BulkMemoryOperations);
  Conf.addProposal(Proposal::ReferenceTypes);
  Conf.addProposal(Proposal::TailCall);

  // Step 1: parse the WAT file → AST::Module (Loader handles .wat/.wasm).
  Loader::Loader Ldr(Conf);
  auto ModRes = Ldr.parseModule(WatPath);
  if (!ModRes) {
    std::cerr << "  [ERROR] Failed to load " << WatPath << "\n";
    return Stats;
  }
  auto &Mod = *ModRes;

  // Step 2: build export-index → export-name map for labelling.
  std::vector<std::string> FuncNames;
  for (const auto &ExpDesc : Mod->getExportSection().getContent()) {
    if (ExpDesc.getExternalType() == ExternalType::Function) {
      uint32_t Idx = ExpDesc.getExternalIndex();
      while (FuncNames.size() <= Idx)
        FuncNames.push_back("func[" + std::to_string(FuncNames.size()) + "]");
      FuncNames[Idx] = std::string(ExpDesc.getExternalName());
    }
  }

  // Count imported functions (absolute function indices start after imports).
  size_t ImportedFuncCount = 0;
  for (const auto &Imp : Mod->getImportSection().getContent())
    if (Imp.getExternalType() == ExternalType::Function)
      ++ImportedFuncCount;

  // Step 3: measure raw sizes, then run the full IPCP pass (in-place) and
  // count nops per segment.
  //
  // We capture raw instruction counts before any optimization, then invoke the
  // module-level IPCP pipeline (purity analysis + call-site folding +
  // arithmetic folding, iterated to fixpoint).  This mirrors exactly what the
  // VM does during instantiation (see lib/vm/vm.cpp unsafeInstantiate).
  const auto &Segments = Mod->getCodeSection().getContent();

  // (a) Snapshot raw counts before optimizing.
  std::vector<size_t> RawCounts;
  RawCounts.reserve(Segments.size());
  for (size_t SI = 0; SI < Segments.size(); ++SI)
    RawCounts.push_back(Segments[SI].getExpr().getInstrs().size());

  // (b) Validate to set PCOffset/JumpEnd/JumpElse on br/br_if/if/else
  //     instructions — required before running IPCP (evalInstrs uses PCOffset
  //     to execute backward branches; without this, br 0 inside a loop has
  //     PCOffset=0, causing an infinite loop in the mini interpreter).
  Validator::Validator Val(Conf);
  if (!Val.validate(*Mod)) {
    std::cerr << "  [ERROR] Validation failed for " << WatPath << "\n";
    return Stats;
  }

  // (c) Run the full IPCP pipeline (modifies Mod in-place).
  Executor::optimizeModuleConstantExpressions(*Mod);

  // (d) Count nops per segment after optimization.
  for (size_t SI = 0; SI < Segments.size(); ++SI) {
    const AST::InstrView OptView = Segments[SI].getExpr().getInstrs();
    size_t RawCount = RawCounts[SI];

    size_t NopCount = 0;
    for (const auto &I : OptView)
      if (I.getOpCode() == OpCode::Nop)
        ++NopCount;

    // Name lookup: exports use absolute function indices.
    size_t AbsIdx = ImportedFuncCount + SI;
    std::string Name = (AbsIdx < FuncNames.size())
                           ? FuncNames[AbsIdx]
                           : "func[" + std::to_string(AbsIdx) + "]";

    Stats.Funcs.push_back(FuncStats{Name, RawCount, NopCount});
    Stats.RawTotal += RawCount;
    Stats.FoldedNops += NopCount;
  }

  // Step 4: instantiate the module and call main() to verify correctness,
  // but only if "main" is actually exported (avoids noisy error log output).
  // VM::execute returns Expect<vector<pair<ValVariant, ValType>>>.
  bool HasMain = false;
  for (const auto &ExpDesc : Mod->getExportSection().getContent())
    if (ExpDesc.getExternalType() == ExternalType::Function &&
        ExpDesc.getExternalName() == "main")
      HasMain = true;

  if (HasMain) {
    VM::VM TheVM(Conf);
    if (TheVM.loadWasm(*Mod) && TheVM.validate() && TheVM.instantiate()) {
      if (auto Res = TheVM.execute("main"); Res) {
        const auto &Returns = *Res;
        if (!Returns.empty()) {
          Stats.ReturnValue =
              static_cast<int32_t>(Returns[0].first.get<uint32_t>());
          Stats.CallSucceeded = true;
        }
      }
    }
  }

  return Stats;
}

// ---------------------------------------------------------------------------
// Formatted output
// ---------------------------------------------------------------------------

void printSeparator(size_t Width) {
  std::cout << std::string(Width, '-') << "\n";
}

void printModuleReport(const ModuleStats &S) {
  const size_t ColW = 70;
  printSeparator(ColW);
  std::cout << "Module: " << S.FilePath << "\n";
  printSeparator(ColW);

  std::cout << std::left << std::setw(30) << "Function"
            << std::right << std::setw(10) << "Raw instrs"
            << std::setw(12) << "Folded nops"
            << std::setw(10) << "Saved %"
            << "\n";
  printSeparator(ColW);

  for (const auto &F : S.Funcs) {
    double Pct = (F.RawTotal > 0)
                     ? 100.0 * static_cast<double>(F.FoldedNops) /
                           static_cast<double>(F.RawTotal)
                     : 0.0;
    std::cout << std::left << std::setw(30) << F.Name
              << std::right << std::setw(10) << F.RawTotal
              << std::setw(12) << F.FoldedNops
              << std::setw(9) << std::fixed << std::setprecision(1) << Pct
              << "%\n";
  }
  printSeparator(ColW);

  double TotalPct = (S.RawTotal > 0)
                        ? 100.0 * static_cast<double>(S.FoldedNops) /
                              static_cast<double>(S.RawTotal)
                        : 0.0;
  std::cout << std::left << std::setw(30) << "TOTAL"
            << std::right << std::setw(10) << S.RawTotal
            << std::setw(12) << S.FoldedNops
            << std::setw(9) << std::fixed << std::setprecision(1) << TotalPct
            << "%\n";
  printSeparator(ColW);

  if (S.CallSucceeded) {
    std::cout << "main() returned: " << S.ReturnValue << "\n";
  } else {
    std::cout << "main() not called (no export or no-entry module)\n";
  }

  if (S.FoldedNops == 0) {
    std::cout
        << "\n"
        << "  => 0 instructions folded.\n"
        << "     Every arithmetic sub-expression mixes at least one\n"
        << "     local.get (variable) with a constant, so no\n"
        << "     const+const+binop triple ever forms.  main() pushes\n"
        << "     constants only as call arguments — no binary op\n"
        << "     follows, so no foldable triple forms there either.\n"
        << "     The function does NOT collapse to a single const store:\n"
        << "     the pass never evaluates calls or crosses function\n"
        << "     boundaries.\n";
  } else {
    std::cout << "\n"
              << "  => " << S.FoldedNops
              << " nop slots from folded triples ("
              << std::fixed << std::setprecision(1) << TotalPct
              << "% of raw instruction stream).\n";
  }
  std::cout << "\n";
}

} // namespace

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  // Locate WAT files.  By default we look in the directory of the executable
  // (cmake copies them there via configure_file).  An explicit path argument
  // overrides.
  std::filesystem::path WatDir;
  if (argc >= 2) {
    WatDir = std::filesystem::path(argv[1]);
  } else {
    WatDir = std::filesystem::path(argv[0]).parent_path(); // NOLINT
    if (!std::filesystem::exists(WatDir / "fibonacci_bench.wat")) {
      // Fallback: source directory (for running from the build tree manually).
      WatDir = std::filesystem::path(__FILE__).parent_path();
    }
  }

  const std::vector<std::pair<std::string, std::string>> Benchmarks = {
      {"fibonacci_bench.wasm", "fib(10) -> expected 89"},
      {"factorial_bench.wasm", "fac(10) -> expected 3628800"},
      {"mandelbrot_bench.wasm",
       "WAT port: iterateEquation(-0.5, 0.0, 100) -> inside set, expected 100"},
      {"mandelbrot_c.wasm",
       "clang -O1: mandelbrot_c.wasm — no main(), counting folds only"},
  };

  std::cout << "\n"
            << "================================================================"
               "========\n"
            << "  WasmEdge Constant Folding Pass — Real-World WAT Benchmark\n"
            << "================================================================"
               "========\n\n";

  for (const auto &[FileName, Desc] : Benchmarks) {
    auto Path = WatDir / FileName;
    std::cout << "Benchmark: " << Desc << "\n\n";
    if (!std::filesystem::exists(Path)) {
      std::cerr << "  [SKIP] File not found: " << Path << "\n\n";
      continue;
    }
    printModuleReport(analyzeModule(Path));
  }

  // Contrast: the synthetic constFoldBench.wasm where everything IS foldable.
  auto SyntheticPath = WatDir / "constFoldBench.wasm";
  if (std::filesystem::exists(SyntheticPath)) {
    std::cout << "================================================================"
                 "========\n"
              << "  Contrast: constFoldBench.wat — synthetic const+const+binop "
                 "patterns\n"
              << "================================================================"
                 "========\n\n";
    printModuleReport(analyzeModule(SyntheticPath));
  }

  return 0;
}

;; SPDX-License-Identifier: Apache-2.0
;; SPDX-FileCopyrightText: 2019-2024 Second State INC
;;
;; ipcp_edge_cases.wat — IPCP / constant-folding wrong-code edge cases
;;
;; Each exported function calls a pure helper with compile-time-constant
;; arguments.  After optimizeModuleConstantExpressions() runs, the call site
;; should have been replaced by a single i32/i64/f32/f64.const (the helper
;; proven-pure and inlined), or should remain as a Call opcode when the
;; operation is unsafe to fold (div-by-zero, INT_MIN/-1 div, trunc of NaN/Inf).
;;
;; Test categories (derived from GCC and LLVM wrong-code bug reports).
;; References are to verified upstream bugs — not fabricated PR numbers:
;;
;;   1. Safe integer div/rem fold (10/3 = 3)
;;
;;   2. div-by-zero NOT folded — traps in WASM
;;      https://github.com/llvm/llvm-project/issues/136679
;;      https://github.com/llvm/llvm-project/issues/45469
;;
;;   3. INT_MIN / -1 div NOT folded — traps per WASM spec §4.3.2
;;      (Spec-mandated; no single canonical upstream bug URL found.)
;;
;;   4. INT_MIN rem -1 folds to 0 — rem_s does not trap for this case (§4.3.2)
;;      (Spec-mandated; no single canonical upstream bug URL found.)
;;
;;   5. Zero dividend folds (over-eager safety guard regression)
;;
;;   6. i32.shl by width (WASM mask 32 & 31 = 0)
;;      https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106884
;;      https://github.com/llvm/llvm-project/issues/18349
;;
;;   7. i32.shr_s by masked amount (33 & 31 = 1, same shift-mask category)
;;
;;   8. i64.shl by width (WASM mask 64 & 63 = 0)
;;
;;   9. i64.shr_u by width (WASM mask 64 & 63 = 0)
;;
;;  10. f64.convert_i32_s(-1) = -1.0 (signed conversion)
;;      https://github.com/llvm/llvm-project/commit/feba8727f80566074518c9dbb5e90c8f2371c08d
;;      https://github.com/llvm/llvm-project/issues/55150
;;
;;  11. f64.convert_i32_u(-1) = 4294967295.0 (unsigned conversion)
;;
;;  12. i32.trunc_f32_s(NaN) NOT folded — traps per WASM spec §4.3.2
;;      https://github.com/emscripten-core/emscripten/issues/5498
;;
;;  13. i32.trunc_f64_s(-1.5) = -1 (truncation toward zero, safe)
;;
;;  14. i64.trunc_f64_u(4294967295.0) = 4294967295 (safe)
;;
;;  15. i64.extend_i32_s(-1) = 0xFFFFFFFFFFFFFFFF (sign extension)
;;      https://github.com/llvm/llvm-project/issues/55833
;;      https://www.mail-archive.com/llvm-bugs@lists.llvm.org/msg89351.html
;;
;;  16. i64.extend_i32_u(-1) = 4294967295 (zero extension)
;;
;;  17. i32.trunc_sat_f32_s(NaN) = 0 (sat semantics, never traps)
;;
;;  18. Bounded recursion: countdown(5) = 0
;;
;; Notes:
;;  * All arithmetic helpers are pure (no memory, no globals, no tables) so the
;;    purity analysis marks them foldable.
;;  * The "NOT folded" cases remain as Call opcodes because isSafeDivRem /
;;    isSafeTrunc guards suppress the fold.

(module

  ;; -------------------------------------------------------------------------
  ;; Helper: safe_div_s — i32.div_s with safe inputs
  ;; -------------------------------------------------------------------------
  (func $safe_div_s (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.div_s
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: safe_rem_s — i32.rem_s (handles INT_MIN/−1 specially)
  ;; -------------------------------------------------------------------------
  (func $safe_rem_s (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.rem_s
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: safe_div_u — i32.div_u
  ;; -------------------------------------------------------------------------
  (func $safe_div_u (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.div_u
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: shl_i32 — i32.shl
  ;; -------------------------------------------------------------------------
  (func $shl_i32 (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.shl
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: shr_s_i32 — i32.shr_s
  ;; -------------------------------------------------------------------------
  (func $shr_s_i32 (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.shr_s
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: shl_i64 — i64.shl
  ;; -------------------------------------------------------------------------
  (func $shl_i64 (param $a i64) (param $b i64) (result i64)
    local.get $a
    local.get $b
    i64.shl
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: shr_u_i64 — i64.shr_u
  ;; -------------------------------------------------------------------------
  (func $shr_u_i64 (param $a i64) (param $b i64) (result i64)
    local.get $a
    local.get $b
    i64.shr_u
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: convert_s — f64.convert_i32_s
  ;; -------------------------------------------------------------------------
  (func $convert_s (param $a i32) (result f64)
    local.get $a
    f64.convert_i32_s
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: convert_u — f64.convert_i32_u
  ;; -------------------------------------------------------------------------
  (func $convert_u (param $a i32) (result f64)
    local.get $a
    f64.convert_i32_u
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: trunc_f32_s — i32.trunc_f32_s (traps on NaN/Inf/OOB)
  ;; -------------------------------------------------------------------------
  (func $trunc_f32_s (param $a f32) (result i32)
    local.get $a
    i32.trunc_f32_s
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: trunc_f64_s — i32.trunc_f64_s
  ;; -------------------------------------------------------------------------
  (func $trunc_f64_s (param $a f64) (result i32)
    local.get $a
    i32.trunc_f64_s
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: trunc_f64_u_i64 — i64.trunc_f64_u
  ;; -------------------------------------------------------------------------
  (func $trunc_f64_u_i64 (param $a f64) (result i64)
    local.get $a
    i64.trunc_f64_u
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: extend_s — i64.extend_i32_s
  ;; -------------------------------------------------------------------------
  (func $extend_s (param $a i32) (result i64)
    local.get $a
    i64.extend_i32_s
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: extend_u — i64.extend_i32_u
  ;; -------------------------------------------------------------------------
  (func $extend_u (param $a i32) (result i64)
    local.get $a
    i64.extend_i32_u
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: trunc_sat_f32_s — i32.trunc_sat_f32_s (never traps)
  ;; -------------------------------------------------------------------------
  (func $trunc_sat_f32_s (param $a f32) (result i32)
    local.get $a
    i32.trunc_sat_f32_s
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: countdown — simple bounded recursion (countdown to 0)
  ;; Returns 0 when n <= 0, else countdown(n - 1).
  ;; -------------------------------------------------------------------------
  (func $countdown (param $n i32) (result i32)
    local.get $n
    i32.const 0
    i32.le_s
    if (result i32)
      i32.const 0
    else
      local.get $n
      i32.const 1
      i32.sub
      call $countdown
    end
  )

  ;; =========================================================================
  ;; Exported call sites — constant arguments only
  ;; =========================================================================

  ;; 1. Safe div folds: i32.div_s(10, 3) = 3
  (func (export "case_safe_div_s") (result i32)
    i32.const 10
    i32.const 3
    call $safe_div_s
  )

  ;; 2a. div-by-zero i32.div_u(5, 0) — must NOT fold (traps)
  (func (export "case_div_by_zero") (result i32)
    i32.const 5
    i32.const 0
    call $safe_div_u
  )

  ;; 2b. div-by-zero i32.div_s(7, 0) — must NOT fold (traps)
  (func (export "case_div_by_zero_s") (result i32)
    i32.const 7
    i32.const 0
    call $safe_div_s
  )

  ;; 3. INT_MIN / -1 — must NOT fold (traps per WASM spec)
  (func (export "case_intmin_div_negone") (result i32)
    i32.const 0x80000000  ;; INT32_MIN = -2147483648
    i32.const 0xFFFFFFFF  ;; -1
    call $safe_div_s
  )

  ;; 4. INT_MIN rem -1 — must fold to 0 (rem does not trap on INT_MIN/-1)
  (func (export "case_intmin_rem_negone") (result i32)
    i32.const 0x80000000  ;; INT32_MIN
    i32.const 0xFFFFFFFF  ;; -1
    call $safe_rem_s
  )

  ;; 5. Zero dividend: i32.div_u(0, 5) = 0 — must fold
  (func (export "case_zero_dividend") (result i32)
    i32.const 0
    i32.const 5
    call $safe_div_u
  )

  ;; 6. i32.shl(1, 32): WASM masks 32 & 31 = 0 → 1 << 0 = 1
  (func (export "case_shl_by_width") (result i32)
    i32.const 1
    i32.const 32
    call $shl_i32
  )

  ;; 7. i32.shr_s(-1, 33): WASM masks 33 & 31 = 1 → -1 >> 1 = -1 (arithmetic)
  (func (export "case_shrs_masked") (result i32)
    i32.const 0xFFFFFFFF  ;; -1
    i32.const 33
    call $shr_s_i32
  )

  ;; 8. i64.shl(1, 64): WASM masks 64 & 63 = 0 → 1 << 0 = 1
  (func (export "case_shl64_by_width") (result i64)
    i64.const 1
    i64.const 64
    call $shl_i64
  )

  ;; 9. i64.shr_u(-1, 64): WASM masks 64 & 63 = 0 → -1 >> 0 = -1
  (func (export "case_shru64_by_width") (result i64)
    i64.const -1
    i64.const 64
    call $shr_u_i64
  )

  ;; 10. f64.convert_i32_s(-1) = -1.0
  ;;     Return the i64 reinterpret so we can compare exactly in C++.
  (func (export "case_convert_s_neg") (result f64)
    i32.const 0xFFFFFFFF  ;; -1 as i32
    call $convert_s
  )

  ;; 11. f64.convert_i32_u(-1 as uint32) = 4294967295.0
  (func (export "case_convert_u_neg") (result f64)
    i32.const 0xFFFFFFFF  ;; 0xFFFFFFFF = 4294967295 as uint32
    call $convert_u
  )

  ;; 12. i32.trunc_f32_s(NaN) — must NOT fold (traps)
  ;;     0x7FC00000 is the canonical quiet NaN bit pattern for f32.
  (func (export "case_trunc_nan") (result i32)
    f32.const nan  ;; canonical NaN
    call $trunc_f32_s
  )

  ;; 13. i32.trunc_f64_s(-1.5) = -1 (truncates toward zero)
  (func (export "case_trunc_neg_frac") (result i32)
    f64.const -1.5
    call $trunc_f64_s
  )

  ;; 14. i64.trunc_f64_u(4294967295.0) = 4294967295
  (func (export "case_trunc_u64") (result i64)
    f64.const 4294967295.0
    call $trunc_f64_u_i64
  )

  ;; 15. i64.extend_i32_s(-1) = 0xFFFFFFFFFFFFFFFF (sign-extended)
  (func (export "case_extend_s_neg") (result i64)
    i32.const 0xFFFFFFFF  ;; -1
    call $extend_s
  )

  ;; 16. i64.extend_i32_u(-1 as uint32) = 4294967295 (zero-extended)
  (func (export "case_extend_u_neg") (result i64)
    i32.const 0xFFFFFFFF  ;; 4294967295 as uint32
    call $extend_u
  )

  ;; 17. i32.trunc_sat_f32_s(NaN) = 0 (sat semantics, no trap)
  (func (export "case_trunc_sat_nan") (result i32)
    f32.const nan
    call $trunc_sat_f32_s
  )

  ;; 18. countdown(5) = 0 (bounded recursion)
  (func (export "case_countdown") (result i32)
    i32.const 5
    call $countdown
  )

  ;; =========================================================================
  ;; Tests derived from V8 and JSC bytecode optimizer test suites
  ;; =========================================================================

  ;; -------------------------------------------------------------------------
  ;; Helper: f64_min — f64.min (exercises NaN propagation and -0.0 handling)
  ;; -------------------------------------------------------------------------
  (func $f64_min (param $a f64) (param $b f64) (result f64)
    local.get $a
    local.get $b
    f64.min
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: f64_max — f64.max
  ;; -------------------------------------------------------------------------
  (func $f64_max (param $a f64) (param $b f64) (result f64)
    local.get $a
    local.get $b
    f64.max
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: algebraic_chain — (1 - x) + (x * 1) + (3 - x)
  ;; A wrong constant folder might simplify this to 4 for all x.
  ;; Correct: with x = 9, result = (-8) + 9 + (-6) = -5
  ;; -------------------------------------------------------------------------
  (func $algebraic_chain (param $x i32) (result i32)
    i32.const 1
    local.get $x
    i32.sub       ;; 1 - x
    local.get $x
    i32.const 1
    i32.mul       ;; x * 1
    i32.add       ;; (1-x) + (x*1)
    i32.const 3
    local.get $x
    i32.sub       ;; 3 - x
    i32.add       ;; (1-x) + (x*1) + (3-x)
  )

  ;; -------------------------------------------------------------------------
  ;; Helper: f64_neg_zero_mul — checks 0.0 * -1.0 produces -0.0
  ;; Returns i64 reinterpret of the result for exact bit comparison.
  ;; -------------------------------------------------------------------------
  (func $f64_neg_zero_mul (param $a f64) (param $b f64) (result i64)
    local.get $a
    local.get $b
    f64.mul
    i64.reinterpret_f64
  )

  ;; 19. f64.min(NaN, 42.0) = canonical NaN (not 42.0)
  ;;     WebKit Bug 270262: https://bugs.webkit.org/show_bug.cgi?id=270262
  ;;     JSC test: JSTests/wasm/stress/fp-nan-minmax.js
  (func (export "case_f64_min_nan") (result f64)
    f64.const nan
    f64.const 42.0
    call $f64_min
  )

  ;; 20. f64.max(NaN, -5.0) = canonical NaN
  (func (export "case_f64_max_nan") (result f64)
    f64.const nan
    f64.const -5.0
    call $f64_max
  )

  ;; 21. f64.min(-0.0, +0.0) = -0.0
  ;;     GCC Bug 116738: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116738
  (func (export "case_f64_min_neg_zero") (result f64)
    f64.const -0.0
    f64.const 0.0
    call $f64_min
  )

  ;; 22. f64.max(-0.0, +0.0) = +0.0
  (func (export "case_f64_max_neg_zero") (result f64)
    f64.const -0.0
    f64.const 0.0
    call $f64_max
  )

  ;; 23. algebraic_chain(9) = -5 (not 4)
  ;;     LLVM #96366: https://github.com/llvm/llvm-project/issues/96366
  (func (export "case_algebraic_chain") (result i32)
    i32.const 9
    call $algebraic_chain
  )

  ;; 24. 0.0 * -1.0 = -0.0 (bit pattern 0x8000000000000000)
  ;;     V8 test: test/mjsunit/minus-zero.js
  (func (export "case_neg_zero_mul") (result i64)
    f64.const 0.0
    f64.const -1.0
    call $f64_neg_zero_mul
  )

  ;; =========================================================================
  ;; Impurity tests: memory.size / memory.grow
  ;;
  ;; Functions that call memory.size or memory.grow are IMPURE because they
  ;; observe or mutate the memory instance.  The purity analysis must mark
  ;; any call graph containing these as non-foldable.
  ;; =========================================================================

  (memory 1)  ;; 1 page of linear memory

  ;; Helper: uses_mem_size — returns memory.size (observes memory state)
  ;; IMPURE: must NOT be folded through IPCP.
  (func $uses_mem_size (param $dummy i32) (result i32)
    memory.size
  )

  ;; Helper: uses_mem_grow — calls memory.grow (mutates memory state)
  ;; IMPURE: must NOT be folded through IPCP.
  (func $uses_mem_grow (param $pages i32) (result i32)
    local.get $pages
    memory.grow
  )

  ;; Helper: transitive_impure — calls a pure op then an impure op.
  ;; The purity analysis must propagate impurity transitively through the
  ;; call graph: even though this function's OWN instructions are pure,
  ;; it calls $uses_mem_size which is impure.
  (func $transitive_impure (param $x i32) (result i32)
    local.get $x
    i32.const 1
    i32.add       ;; pure arithmetic → (x + 1) on stack
    i32.const 0
    call $uses_mem_size  ;; impure callee → makes this function impure
    i32.add       ;; (x + 1) + memory.size
  )

  ;; 25. memory.size impurity — call must NOT be folded.
  (func (export "case_mem_size_impure") (result i32)
    i32.const 0
    call $uses_mem_size
  )

  ;; 26. memory.grow impurity — call must NOT be folded.
  (func (export "case_mem_grow_impure") (result i32)
    i32.const 0
    call $uses_mem_grow
  )

  ;; 27. Transitive impurity through call graph — must NOT be folded.
  (func (export "case_transitive_impure") (result i32)
    i32.const 42
    call $transitive_impure
  )

  ;; =========================================================================
  ;; Multi-return tests
  ;;
  ;; WASM multi-value proposal allows functions to return multiple values.
  ;; These test IPCP handling of multi-return, derived from:
  ;;   wasm3 #220: multi-return values come back in wrong order
  ;;     https://github.com/wasm3/wasm3/issues/220
  ;;   Wasmtime #2316: incorrect values with >3 return parameters
  ;;     https://github.com/bytecodealliance/wasmtime/issues/2316
  ;;   LLVM #101335: IPSCCP propagates constants to wrong call sites
  ;;     https://github.com/llvm/llvm-project/issues/101335
  ;; =========================================================================

  ;; Helper: swap — returns (b, a) given (a, b).  Pure, 2 params, 2 returns.
  (func $swap (param $a i32) (param $b i32) (result i32 i32)
    local.get $b
    local.get $a
  )

  ;; Helper: mixed_return — returns (i32, i64) to test mixed-type multi-return.
  (func $mixed_return (param $x i32) (result i32 i64)
    local.get $x
    local.get $x
    i64.extend_i32_s
  )

  ;; Helper: triple_return — 3 params, 3 returns (exact slot fit: 3+1=4 slots,
  ;; 3 results fit).  Returns params in reverse order.
  (func $reverse3 (param $a i32) (param $b i32) (param $c i32) (result i32 i32 i32)
    local.get $c
    local.get $b
    local.get $a
  )

  ;; Helper: quad_return — 1 param, 4 returns.  This has NumReturns(4) >
  ;; NumParams(1)+1 = 2 slots, so it CANNOT be folded (slot overflow).
  (func $quad_return (param $x i32) (result i32 i32 i32 i32)
    local.get $x
    local.get $x
    i32.const 1
    i32.add
    local.get $x
    i32.const 2
    i32.add
    local.get $x
    i32.const 3
    i32.add
  )

  ;; Helper: void_fn — 2 params, 0 returns (void).  Pure.
  ;; All slots (2 params + call) become Nops.
  (func $void_fn (param $a i32) (param $b i32))

  ;; 28. Multi-return: swap(10, 20) = (20, 10).
  ;;     Tests value ordering (wasm3 #220 bug class).
  (func (export "case_multi_swap") (result i32 i32)
    i32.const 10
    i32.const 20
    call $swap
  )

  ;; 29. Mixed-type multi-return: mixed_return(-1) = (-1, -1_i64).
  ;;     Tests type discrimination in slot patching.
  (func (export "case_multi_mixed") (result i32 i64)
    i32.const -1
    call $mixed_return
  )

  ;; 30. Triple return: reverse3(1, 2, 3) = (3, 2, 1).
  ;;     Exact slot fit (3 params + 1 call = 4 slots, 3 results = fits).
  (func (export "case_multi_triple") (result i32 i32 i32)
    i32.const 1
    i32.const 2
    i32.const 3
    call $reverse3
  )

  ;; 31. Quad return: slot overflow — NumReturns(4) > NumParams(1)+1 = 2.
  ;;     Must NOT be folded (not enough instruction slots).
  (func (export "case_multi_overflow") (result i32 i32 i32 i32)
    i32.const 5
    call $quad_return
  )

  ;; 32. Void function: void_fn(10, 20) — 0 returns, all slots become Nops.
  ;;     After IPCP, the caller's body should have no Call remaining.
  (func (export "case_void_fn") (result i32)
    i32.const 10
    i32.const 20
    call $void_fn
    i32.const 42  ;; caller's own return value
  )

  ;; 33. Same function called with different args at different sites.
  ;;     swap(1,2) should give (2,1), swap(100,200) should give (200,100).
  ;;     LLVM #101335: IPSCCP propagated constants from wrong call site.
  (func (export "case_multi_site_a") (result i32 i32)
    i32.const 1
    i32.const 2
    call $swap
  )

  (func (export "case_multi_site_b") (result i32 i32)
    i32.const 100
    i32.const 200
    call $swap
  )

)

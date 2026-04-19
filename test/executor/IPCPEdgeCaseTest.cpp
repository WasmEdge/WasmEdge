// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/executor/IPCPEdgeCaseTest.cpp -- IPCP edge cases ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// End-to-end tests for the IPCP (Inter-Procedural Constant Propagation)
/// constant-folding pass on a compiled WASM module (ipcp_edge_cases.wasm).
///
/// These tests are derived from GCC and LLVM wrong-code bug reports for their
/// IPCP / constant-folding passes, translated into WebAssembly.  Each test
/// case verifies either:
///   (a) that the call site WAS folded (no Call opcode remains, correct value),
///   (b) or that the call site was NOT folded (Call opcode preserved) because
///       the operation would trap or is otherwise unsafe.
///
/// Test categories covered (with references to real confirmed upstream bugs):
///
///   1.  Safe i32.div_s fold (10/3 = 3) — guard must not over-suppress.
///
///   2.  div-by-zero NOT folded — WASM traps, fold must be suppressed.
///       https://github.com/llvm/llvm-project/issues/136679
///       https://github.com/llvm/llvm-project/issues/45469
///
///   3.  INT_MIN / -1 div NOT folded — traps per WASM Core Spec §4.3.2.
///       (Behaviour is mandated by the spec; no single canonical upstream
///        bug URL was identified for this precise WASM IPCP scenario.)
///
///   4.  INT_MIN rem -1 folds to 0 — rem_s does NOT trap here per §4.3.2.
///       (Spec-mandated; no single canonical upstream bug URL found.)
///
///   5.  Zero dividend folds (0 / non-zero = 0) — regression guard.
///
///   6.  i32.shl by width — WASM mask 32 & 31 = 0, result = 1 (not 0).
///       https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106884
///       https://github.com/llvm/llvm-project/issues/18349
///
///   7.  i32.shr_s by masked amount — 33 & 31 = 1 (same shift-mask category).
///
///   8.  i64.shl by width — WASM mask 64 & 63 = 0.
///
///   9.  i64.shr_u by width — WASM mask 64 & 63 = 0.
///
///  10.  f64.convert_i32_s(-1) = -1.0 — signed conversion path.
///       https://github.com/llvm/llvm-project/commit/feba8727f80566074518c9dbb5e90c8f2371c08d
///       https://github.com/llvm/llvm-project/issues/55150
///
///  11.  f64.convert_i32_u(0xFFFFFFFF) = 4294967295.0 — unsigned path.
///       (Same category as above; wrong path gives -1.0.)
///
///  12.  i32.trunc_f32_s(NaN) NOT folded — WASM traps on NaN.
///       https://github.com/emscripten-core/emscripten/issues/5498
///
///  13.  i32.trunc_f64_s(-1.5) = -1 — safe value must fold correctly.
///
///  14.  i64.trunc_f64_u(4294967295.0) = 4294967295 — safe value folds.
///
///  15.  i64.extend_i32_s(-1) = 0xFFFFFFFFFFFFFFFF — sign extension.
///       https://github.com/llvm/llvm-project/issues/55833
///       https://www.mail-archive.com/llvm-bugs@lists.llvm.org/msg89351.html
///
///  16.  i64.extend_i32_u(0xFFFFFFFF) = 4294967295 — zero extension.
///       (Wrong fold using signed path gives 0xFFFFFFFFFFFFFFFF.)
///
///  17.  i32.trunc_sat_f32_s(NaN) = 0 — sat semantics, no trap, must fold.
///
///  18.  Bounded recursion: countdown(5) = 0 — IPCP with depth limit.
///
//===----------------------------------------------------------------------===//

#include "ast/instruction.h"
#include "ast/module.h"
#include "common/configure.h"
#include "common/enum_ast.hpp"
#include "common/enum_types.hpp"
#include "common/types.h"
#include "executor/engine/const_fold.h"
#include "loader/loader.h"
#include "validator/validator.h"

#include "gtest/gtest.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <vector>

using namespace WasmEdge;

// ---------------------------------------------------------------------------
// Inline WASM binary for ipcp_edge_cases.wat.
//
// Source: test/executor/ipcp_edge_cases.wat (compiled via wat2wasm).
// To regenerate: wat2wasm ipcp_edge_cases.wat -o /dev/stdout | xxd -i
//
// This follows the WasmEdge convention of embedding test WASM modules as
// inline binary arrays (see ExecutorRegressionTest.cpp for the pattern).
// The WAT source is kept alongside for human-readable reference.
// ---------------------------------------------------------------------------

// clang-format off
static const std::array<WasmEdge::Byte, 1571> IPCPEdgeCaseWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x7d, 0x16, 0x60,
    0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x60, 0x02, 0x7e, 0x7e, 0x01, 0x7e, 0x60,
    0x01, 0x7f, 0x01, 0x7c, 0x60, 0x01, 0x7d, 0x01, 0x7f, 0x60, 0x01, 0x7c,
    0x01, 0x7f, 0x60, 0x01, 0x7c, 0x01, 0x7e, 0x60, 0x01, 0x7f, 0x01, 0x7e,
    0x60, 0x01, 0x7f, 0x01, 0x7f, 0x60, 0x00, 0x01, 0x7f, 0x60, 0x00, 0x01,
    0x7e, 0x60, 0x00, 0x01, 0x7c, 0x60, 0x02, 0x7c, 0x7c, 0x01, 0x7c, 0x60,
    0x02, 0x7c, 0x7c, 0x01, 0x7e, 0x60, 0x02, 0x7f, 0x7f, 0x02, 0x7f, 0x7f,
    0x60, 0x01, 0x7f, 0x02, 0x7f, 0x7e, 0x60, 0x03, 0x7f, 0x7f, 0x7f, 0x03,
    0x7f, 0x7f, 0x7f, 0x60, 0x01, 0x7f, 0x04, 0x7f, 0x7f, 0x7f, 0x7f, 0x60,
    0x02, 0x7f, 0x7f, 0x00, 0x60, 0x00, 0x02, 0x7f, 0x7f, 0x60, 0x00, 0x02,
    0x7f, 0x7e, 0x60, 0x00, 0x03, 0x7f, 0x7f, 0x7f, 0x60, 0x00, 0x04, 0x7f,
    0x7f, 0x7f, 0x7f, 0x03, 0x40, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x01, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x03, 0x07, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x0a, 0x0a, 0x08, 0x08,
    0x09, 0x09, 0x09, 0x08, 0x08, 0x0b, 0x0b, 0x07, 0x0c, 0x0a, 0x0a, 0x0a,
    0x0a, 0x08, 0x09, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x08, 0x12, 0x12, 0x05, 0x03, 0x01,
    0x00, 0x01, 0x07, 0xd4, 0x05, 0x23, 0x0f, 0x63, 0x61, 0x73, 0x65, 0x5f,
    0x73, 0x61, 0x66, 0x65, 0x5f, 0x64, 0x69, 0x76, 0x5f, 0x73, 0x00, 0x10,
    0x10, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x64, 0x69, 0x76, 0x5f, 0x62, 0x79,
    0x5f, 0x7a, 0x65, 0x72, 0x6f, 0x00, 0x11, 0x12, 0x63, 0x61, 0x73, 0x65,
    0x5f, 0x64, 0x69, 0x76, 0x5f, 0x62, 0x79, 0x5f, 0x7a, 0x65, 0x72, 0x6f,
    0x5f, 0x73, 0x00, 0x12, 0x16, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x69, 0x6e,
    0x74, 0x6d, 0x69, 0x6e, 0x5f, 0x64, 0x69, 0x76, 0x5f, 0x6e, 0x65, 0x67,
    0x6f, 0x6e, 0x65, 0x00, 0x13, 0x16, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x69,
    0x6e, 0x74, 0x6d, 0x69, 0x6e, 0x5f, 0x72, 0x65, 0x6d, 0x5f, 0x6e, 0x65,
    0x67, 0x6f, 0x6e, 0x65, 0x00, 0x14, 0x12, 0x63, 0x61, 0x73, 0x65, 0x5f,
    0x7a, 0x65, 0x72, 0x6f, 0x5f, 0x64, 0x69, 0x76, 0x69, 0x64, 0x65, 0x6e,
    0x64, 0x00, 0x15, 0x11, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x73, 0x68, 0x6c,
    0x5f, 0x62, 0x79, 0x5f, 0x77, 0x69, 0x64, 0x74, 0x68, 0x00, 0x16, 0x10,
    0x63, 0x61, 0x73, 0x65, 0x5f, 0x73, 0x68, 0x72, 0x73, 0x5f, 0x6d, 0x61,
    0x73, 0x6b, 0x65, 0x64, 0x00, 0x17, 0x13, 0x63, 0x61, 0x73, 0x65, 0x5f,
    0x73, 0x68, 0x6c, 0x36, 0x34, 0x5f, 0x62, 0x79, 0x5f, 0x77, 0x69, 0x64,
    0x74, 0x68, 0x00, 0x18, 0x14, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x73, 0x68,
    0x72, 0x75, 0x36, 0x34, 0x5f, 0x62, 0x79, 0x5f, 0x77, 0x69, 0x64, 0x74,
    0x68, 0x00, 0x19, 0x12, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x63, 0x6f, 0x6e,
    0x76, 0x65, 0x72, 0x74, 0x5f, 0x73, 0x5f, 0x6e, 0x65, 0x67, 0x00, 0x1a,
    0x12, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x63, 0x6f, 0x6e, 0x76, 0x65, 0x72,
    0x74, 0x5f, 0x75, 0x5f, 0x6e, 0x65, 0x67, 0x00, 0x1b, 0x0e, 0x63, 0x61,
    0x73, 0x65, 0x5f, 0x74, 0x72, 0x75, 0x6e, 0x63, 0x5f, 0x6e, 0x61, 0x6e,
    0x00, 0x1c, 0x13, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x74, 0x72, 0x75, 0x6e,
    0x63, 0x5f, 0x6e, 0x65, 0x67, 0x5f, 0x66, 0x72, 0x61, 0x63, 0x00, 0x1d,
    0x0e, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x74, 0x72, 0x75, 0x6e, 0x63, 0x5f,
    0x75, 0x36, 0x34, 0x00, 0x1e, 0x11, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x65,
    0x78, 0x74, 0x65, 0x6e, 0x64, 0x5f, 0x73, 0x5f, 0x6e, 0x65, 0x67, 0x00,
    0x1f, 0x11, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x65, 0x78, 0x74, 0x65, 0x6e,
    0x64, 0x5f, 0x75, 0x5f, 0x6e, 0x65, 0x67, 0x00, 0x20, 0x12, 0x63, 0x61,
    0x73, 0x65, 0x5f, 0x74, 0x72, 0x75, 0x6e, 0x63, 0x5f, 0x73, 0x61, 0x74,
    0x5f, 0x6e, 0x61, 0x6e, 0x00, 0x21, 0x0e, 0x63, 0x61, 0x73, 0x65, 0x5f,
    0x63, 0x6f, 0x75, 0x6e, 0x74, 0x64, 0x6f, 0x77, 0x6e, 0x00, 0x22, 0x10,
    0x63, 0x61, 0x73, 0x65, 0x5f, 0x66, 0x36, 0x34, 0x5f, 0x6d, 0x69, 0x6e,
    0x5f, 0x6e, 0x61, 0x6e, 0x00, 0x27, 0x10, 0x63, 0x61, 0x73, 0x65, 0x5f,
    0x66, 0x36, 0x34, 0x5f, 0x6d, 0x61, 0x78, 0x5f, 0x6e, 0x61, 0x6e, 0x00,
    0x28, 0x15, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x66, 0x36, 0x34, 0x5f, 0x6d,
    0x69, 0x6e, 0x5f, 0x6e, 0x65, 0x67, 0x5f, 0x7a, 0x65, 0x72, 0x6f, 0x00,
    0x29, 0x15, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x66, 0x36, 0x34, 0x5f, 0x6d,
    0x61, 0x78, 0x5f, 0x6e, 0x65, 0x67, 0x5f, 0x7a, 0x65, 0x72, 0x6f, 0x00,
    0x2a, 0x14, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x61, 0x6c, 0x67, 0x65, 0x62,
    0x72, 0x61, 0x69, 0x63, 0x5f, 0x63, 0x68, 0x61, 0x69, 0x6e, 0x00, 0x2b,
    0x11, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x6e, 0x65, 0x67, 0x5f, 0x7a, 0x65,
    0x72, 0x6f, 0x5f, 0x6d, 0x75, 0x6c, 0x00, 0x2c, 0x14, 0x63, 0x61, 0x73,
    0x65, 0x5f, 0x6d, 0x65, 0x6d, 0x5f, 0x73, 0x69, 0x7a, 0x65, 0x5f, 0x69,
    0x6d, 0x70, 0x75, 0x72, 0x65, 0x00, 0x30, 0x14, 0x63, 0x61, 0x73, 0x65,
    0x5f, 0x6d, 0x65, 0x6d, 0x5f, 0x67, 0x72, 0x6f, 0x77, 0x5f, 0x69, 0x6d,
    0x70, 0x75, 0x72, 0x65, 0x00, 0x31, 0x16, 0x63, 0x61, 0x73, 0x65, 0x5f,
    0x74, 0x72, 0x61, 0x6e, 0x73, 0x69, 0x74, 0x69, 0x76, 0x65, 0x5f, 0x69,
    0x6d, 0x70, 0x75, 0x72, 0x65, 0x00, 0x32, 0x0f, 0x63, 0x61, 0x73, 0x65,
    0x5f, 0x6d, 0x75, 0x6c, 0x74, 0x69, 0x5f, 0x73, 0x77, 0x61, 0x70, 0x00,
    0x38, 0x10, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x6d, 0x75, 0x6c, 0x74, 0x69,
    0x5f, 0x6d, 0x69, 0x78, 0x65, 0x64, 0x00, 0x39, 0x11, 0x63, 0x61, 0x73,
    0x65, 0x5f, 0x6d, 0x75, 0x6c, 0x74, 0x69, 0x5f, 0x74, 0x72, 0x69, 0x70,
    0x6c, 0x65, 0x00, 0x3a, 0x13, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x6d, 0x75,
    0x6c, 0x74, 0x69, 0x5f, 0x6f, 0x76, 0x65, 0x72, 0x66, 0x6c, 0x6f, 0x77,
    0x00, 0x3b, 0x0c, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x76, 0x6f, 0x69, 0x64,
    0x5f, 0x66, 0x6e, 0x00, 0x3c, 0x11, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x6d,
    0x75, 0x6c, 0x74, 0x69, 0x5f, 0x73, 0x69, 0x74, 0x65, 0x5f, 0x61, 0x00,
    0x3d, 0x11, 0x63, 0x61, 0x73, 0x65, 0x5f, 0x6d, 0x75, 0x6c, 0x74, 0x69,
    0x5f, 0x73, 0x69, 0x74, 0x65, 0x5f, 0x62, 0x00, 0x3e, 0x0a, 0xfb, 0x04,
    0x3f, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6d, 0x0b, 0x07, 0x00, 0x20,
    0x00, 0x20, 0x01, 0x6f, 0x0b, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6e,
    0x0b, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x74, 0x0b, 0x07, 0x00, 0x20,
    0x00, 0x20, 0x01, 0x75, 0x0b, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x86,
    0x0b, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x88, 0x0b, 0x05, 0x00, 0x20,
    0x00, 0xb7, 0x0b, 0x05, 0x00, 0x20, 0x00, 0xb8, 0x0b, 0x05, 0x00, 0x20,
    0x00, 0xa8, 0x0b, 0x05, 0x00, 0x20, 0x00, 0xaa, 0x0b, 0x05, 0x00, 0x20,
    0x00, 0xb1, 0x0b, 0x05, 0x00, 0x20, 0x00, 0xac, 0x0b, 0x05, 0x00, 0x20,
    0x00, 0xad, 0x0b, 0x06, 0x00, 0x20, 0x00, 0xfc, 0x00, 0x0b, 0x14, 0x00,
    0x20, 0x00, 0x41, 0x00, 0x4c, 0x04, 0x7f, 0x41, 0x00, 0x05, 0x20, 0x00,
    0x41, 0x01, 0x6b, 0x10, 0x0f, 0x0b, 0x0b, 0x08, 0x00, 0x41, 0x0a, 0x41,
    0x03, 0x10, 0x00, 0x0b, 0x08, 0x00, 0x41, 0x05, 0x41, 0x00, 0x10, 0x02,
    0x0b, 0x08, 0x00, 0x41, 0x07, 0x41, 0x00, 0x10, 0x00, 0x0b, 0x0c, 0x00,
    0x41, 0x80, 0x80, 0x80, 0x80, 0x78, 0x41, 0x7f, 0x10, 0x00, 0x0b, 0x0c,
    0x00, 0x41, 0x80, 0x80, 0x80, 0x80, 0x78, 0x41, 0x7f, 0x10, 0x01, 0x0b,
    0x08, 0x00, 0x41, 0x00, 0x41, 0x05, 0x10, 0x02, 0x0b, 0x08, 0x00, 0x41,
    0x01, 0x41, 0x20, 0x10, 0x03, 0x0b, 0x08, 0x00, 0x41, 0x7f, 0x41, 0x21,
    0x10, 0x04, 0x0b, 0x09, 0x00, 0x42, 0x01, 0x42, 0xc0, 0x00, 0x10, 0x05,
    0x0b, 0x09, 0x00, 0x42, 0x7f, 0x42, 0xc0, 0x00, 0x10, 0x06, 0x0b, 0x06,
    0x00, 0x41, 0x7f, 0x10, 0x07, 0x0b, 0x06, 0x00, 0x41, 0x7f, 0x10, 0x08,
    0x0b, 0x09, 0x00, 0x43, 0x00, 0x00, 0xc0, 0x7f, 0x10, 0x09, 0x0b, 0x0d,
    0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xbf, 0x10, 0x0a,
    0x0b, 0x0d, 0x00, 0x44, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0xef, 0x41,
    0x10, 0x0b, 0x0b, 0x06, 0x00, 0x41, 0x7f, 0x10, 0x0c, 0x0b, 0x06, 0x00,
    0x41, 0x7f, 0x10, 0x0d, 0x0b, 0x09, 0x00, 0x43, 0x00, 0x00, 0xc0, 0x7f,
    0x10, 0x0e, 0x0b, 0x06, 0x00, 0x41, 0x05, 0x10, 0x0f, 0x0b, 0x07, 0x00,
    0x20, 0x00, 0x20, 0x01, 0xa4, 0x0b, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01,
    0xa5, 0x0b, 0x13, 0x00, 0x41, 0x01, 0x20, 0x00, 0x6b, 0x20, 0x00, 0x41,
    0x01, 0x6c, 0x6a, 0x41, 0x03, 0x20, 0x00, 0x6b, 0x6a, 0x0b, 0x08, 0x00,
    0x20, 0x00, 0x20, 0x01, 0xa2, 0xbd, 0x0b, 0x16, 0x00, 0x44, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x45, 0x40, 0x10, 0x23, 0x0b, 0x16, 0x00, 0x44, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xf8, 0x7f, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x14, 0xc0, 0x10, 0x24, 0x0b, 0x16, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x10, 0x23, 0x0b, 0x16, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x24, 0x0b, 0x06, 0x00, 0x41, 0x09, 0x10, 0x25, 0x0b, 0x16, 0x00,
    0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xf0, 0xbf, 0x10, 0x26, 0x0b, 0x04, 0x00, 0x3f,
    0x00, 0x0b, 0x06, 0x00, 0x20, 0x00, 0x40, 0x00, 0x0b, 0x0c, 0x00, 0x20,
    0x00, 0x41, 0x01, 0x6a, 0x41, 0x00, 0x10, 0x2d, 0x6a, 0x0b, 0x06, 0x00,
    0x41, 0x00, 0x10, 0x2d, 0x0b, 0x06, 0x00, 0x41, 0x00, 0x10, 0x2e, 0x0b,
    0x06, 0x00, 0x41, 0x2a, 0x10, 0x2f, 0x0b, 0x06, 0x00, 0x20, 0x01, 0x20,
    0x00, 0x0b, 0x07, 0x00, 0x20, 0x00, 0x20, 0x00, 0xac, 0x0b, 0x08, 0x00,
    0x20, 0x02, 0x20, 0x01, 0x20, 0x00, 0x0b, 0x13, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x41, 0x01, 0x6a, 0x20, 0x00, 0x41, 0x02, 0x6a, 0x20, 0x00, 0x41,
    0x03, 0x6a, 0x0b, 0x02, 0x00, 0x0b, 0x08, 0x00, 0x41, 0x0a, 0x41, 0x14,
    0x10, 0x33, 0x0b, 0x06, 0x00, 0x41, 0x7f, 0x10, 0x34, 0x0b, 0x0a, 0x00,
    0x41, 0x01, 0x41, 0x02, 0x41, 0x03, 0x10, 0x35, 0x0b, 0x06, 0x00, 0x41,
    0x05, 0x10, 0x36, 0x0b, 0x0a, 0x00, 0x41, 0x0a, 0x41, 0x14, 0x10, 0x37,
    0x41, 0x2a, 0x0b, 0x08, 0x00, 0x41, 0x01, 0x41, 0x02, 0x10, 0x33, 0x0b,
    0x0a, 0x00, 0x41, 0xe4, 0x00, 0x41, 0xc8, 0x01, 0x10, 0x33, 0x0b
};
// clang-format on

// ---------------------------------------------------------------------------
// Test fixture: loads, validates, and optimizes the inline WASM module once.
// ---------------------------------------------------------------------------

class IPCPEdgeCaseTest : public ::testing::Test {
protected:
  // The (validated, IPCP-optimized) module shared across all tests.
  static std::unique_ptr<AST::Module> Mod;

  // Map from export name -> absolute function index.
  static std::map<std::string, uint32_t> ExportMap;

  // Number of imported functions (offset from code-segment index to abs index).
  static size_t ImportedFuncCount;

  // True if module setup succeeded.
  static bool SetupOK;

  static void SetUpTestSuite() {
    Configure Conf;
    Conf.addProposal(Proposal::SIMD);
    Conf.addProposal(Proposal::BulkMemoryOperations);
    Conf.addProposal(Proposal::ReferenceTypes);
    Conf.addProposal(Proposal::TailCall);

    // Load from inline binary array.
    Loader::Loader Ldr(Conf);
    auto ModRes = Ldr.parseModule(IPCPEdgeCaseWasm);
    if (!ModRes) {
      ADD_FAILURE() << "Failed to parse inline IPCPEdgeCaseWasm binary";
      SetupOK = false;
      return;
    }

    // Validate (sets PCOffset on branch instructions -- required by IPCP).
    Validator::Validator Val(Conf);
    if (!Val.validate(**ModRes)) {
      ADD_FAILURE() << "Validation failed for inline IPCPEdgeCaseWasm";
      SetupOK = false;
      return;
    }

    Mod = std::move(*ModRes);

    // Run full IPCP pipeline.
    Executor::optimizeModuleConstantExpressions(*Mod);

    // Build export name → absolute function index map.
    for (const auto &ExpDesc : Mod->getExportSection().getContent()) {
      if (ExpDesc.getExternalType() == ExternalType::Function) {
        ExportMap[std::string(ExpDesc.getExternalName())] =
            ExpDesc.getExternalIndex();
      }
    }

    // Count imported functions.
    ImportedFuncCount = 0;
    for (const auto &Imp : Mod->getImportSection().getContent()) {
      if (Imp.getExternalType() == ExternalType::Function)
        ++ImportedFuncCount;
    }

    SetupOK = true;
  }

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  /// Return the instruction view for a named exported function.
  AST::InstrView getInstrs(const std::string &Name) const {
    auto It = ExportMap.find(Name);
    EXPECT_NE(It, ExportMap.end()) << "Export not found: " << Name;
    if (It == ExportMap.end())
      return {};
    uint32_t AbsIdx = It->second;
    size_t SegIdx = AbsIdx - ImportedFuncCount;
    const auto &Segments = Mod->getCodeSection().getContent();
    EXPECT_LT(SegIdx, Segments.size());
    if (SegIdx >= Segments.size())
      return {};
    return Segments[SegIdx].getExpr().getInstrs();
  }

  /// Returns true if the instruction stream contains no Call opcode
  /// (i.e., the call site was folded away).
  static bool isFolded(const AST::InstrView &Instrs) {
    for (const auto &I : Instrs)
      if (I.getOpCode() == OpCode::Call)
        return false;
    return true;
  }

  /// Returns true if the instruction stream still contains a Call opcode.
  static bool hasCall(const AST::InstrView &Instrs) {
    return !isFolded(Instrs);
  }

  /// Returns the first non-Nop non-End instruction in the stream.
  static std::optional<AST::Instruction> firstConst(const AST::InstrView &Instrs) {
    for (const auto &I : Instrs) {
      if (I.getOpCode() == OpCode::Nop)
        continue;
      if (I.getOpCode() == OpCode::End)
        break;
      return I;
    }
    return std::nullopt;
  }
};

// Static member definitions.
std::unique_ptr<AST::Module> IPCPEdgeCaseTest::Mod;
std::map<std::string, uint32_t> IPCPEdgeCaseTest::ExportMap;
size_t IPCPEdgeCaseTest::ImportedFuncCount = 0;
bool IPCPEdgeCaseTest::SetupOK = false;

// ---------------------------------------------------------------------------
// Sanity check: setup succeeded.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, SetupSucceeded) { ASSERT_TRUE(SetupOK); }

// ---------------------------------------------------------------------------
// 1. Safe i32.div_s(10, 3) = 3 — must fold.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case01_SafeDivSFolds) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_safe_div_s");
  EXPECT_TRUE(isFolded(Instrs)) << "case_safe_div_s: expected call to be folded";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(FC->getNum().get<uint32_t>()), 3);
}

// ---------------------------------------------------------------------------
// 2a. i32.div_u(5, 0) — must NOT fold (traps in WASM).
//     https://github.com/llvm/llvm-project/issues/136679
//     https://github.com/llvm/llvm-project/issues/45469
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case02a_DivByZeroNotFolded) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_div_by_zero");
  EXPECT_TRUE(hasCall(Instrs))
      << "case_div_by_zero: div-by-zero must NOT be folded (traps in WASM)";
}

// ---------------------------------------------------------------------------
// 2b. i32.div_s(7, 0) — must NOT fold.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case02b_DivByZeroSignedNotFolded) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_div_by_zero_s");
  EXPECT_TRUE(hasCall(Instrs))
      << "case_div_by_zero_s: signed div-by-zero must NOT be folded";
}

// ---------------------------------------------------------------------------
// 3. i32.div_s(INT32_MIN, -1) — must NOT fold (traps per WASM spec §4.3.2).
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case03_IntMinDivNegOneNotFolded) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_intmin_div_negone");
  EXPECT_TRUE(hasCall(Instrs))
      << "case_intmin_div_negone: INT32_MIN / -1 must NOT be folded (traps)";
}

// ---------------------------------------------------------------------------
// 4. i32.rem_s(INT32_MIN, -1) = 0 — must fold to 0.
//    WASM spec §4.3.2 ("irem_s"): rem_s does not trap for INT_MIN/−1; result 0.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case04_IntMinRemNegOneFoldsToZero) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_intmin_rem_negone");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_intmin_rem_negone: INT32_MIN rem -1 must fold to 0";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I32__const);
  EXPECT_EQ(FC->getNum().get<uint32_t>(), 0u);
}

// ---------------------------------------------------------------------------
// 5. i32.div_u(0, 5) = 0 — must fold (safety guard must not over-suppress).
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case05_ZeroDividendFolds) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_zero_dividend");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_zero_dividend: 0/5 must fold to 0";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I32__const);
  EXPECT_EQ(FC->getNum().get<uint32_t>(), 0u);
}

// ---------------------------------------------------------------------------
// 6. i32.shl(1, 32): WASM masks 32 & 31 = 0 → 1 (not 0 as C UB would give).
//    https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106884
//    https://github.com/llvm/llvm-project/issues/18349
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case06_ShlByWidth) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_shl_by_width");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_shl_by_width: i32.shl(1, 32) must fold";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I32__const);
  // WASM: 32 & 31 = 0 → 1 << 0 = 1.  A C-UB fold would give 0.
  EXPECT_EQ(FC->getNum().get<uint32_t>(), 1u);
}

// ---------------------------------------------------------------------------
// 7. i32.shr_s(0xFFFFFFFF, 33): WASM masks 33 & 31 = 1 → -1 >> 1 = -1.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case07_ShrsMasked) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_shrs_masked");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_shrs_masked: i32.shr_s(-1, 33) must fold";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I32__const);
  // Arithmetic right shift of -1 by any amount = -1.
  EXPECT_EQ(FC->getNum().get<uint32_t>(), 0xFFFFFFFFu);
}

// ---------------------------------------------------------------------------
// 8. i64.shl(1, 64): WASM masks 64 & 63 = 0 → 1.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case08_Shl64ByWidth) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_shl64_by_width");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_shl64_by_width: i64.shl(1, 64) must fold";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I64__const);
  EXPECT_EQ(FC->getNum().get<uint64_t>(), 1ull);
}

// ---------------------------------------------------------------------------
// 9. i64.shr_u(-1, 64): WASM masks 64 & 63 = 0 → UINT64_MAX.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case09_Shru64ByWidth) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_shru64_by_width");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_shru64_by_width: i64.shr_u(-1, 64) must fold";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I64__const);
  EXPECT_EQ(FC->getNum().get<uint64_t>(), UINT64_MAX);
}

// ---------------------------------------------------------------------------
// 10. f64.convert_i32_s(-1) = -1.0 — signed conversion path.
//     https://github.com/llvm/llvm-project/commit/feba8727f80566074518c9dbb5e90c8f2371c08d
//     https://github.com/llvm/llvm-project/issues/55150
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case10_ConvertSignedNeg) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_convert_s_neg");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_convert_s_neg: f64.convert_i32_s(-1) must fold";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::F64__const);
  EXPECT_DOUBLE_EQ(FC->getNum().get<double>(), -1.0);
}

// ---------------------------------------------------------------------------
// 11. f64.convert_i32_u(0xFFFFFFFF) = 4294967295.0 — unsigned conversion.
//     A fold using signed interpretation would produce -1.0.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case11_ConvertUnsignedNeg) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_convert_u_neg");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_convert_u_neg: f64.convert_i32_u(0xFFFFFFFF) must fold";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::F64__const);
  EXPECT_DOUBLE_EQ(FC->getNum().get<double>(), 4294967295.0);
}

// ---------------------------------------------------------------------------
// 12. i32.trunc_f32_s(NaN) — must NOT fold (traps in WASM per §4.3.2).
//     https://github.com/emscripten-core/emscripten/issues/5498
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case12_TruncNaNNotFolded) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_trunc_nan");
  EXPECT_TRUE(hasCall(Instrs))
      << "case_trunc_nan: trunc(NaN) must NOT be folded (traps in WASM)";
}

// ---------------------------------------------------------------------------
// 13. i32.trunc_f64_s(-1.5) = -1 — truncates toward zero.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case13_TruncNegFrac) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_trunc_neg_frac");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_trunc_neg_frac: i32.trunc_f64_s(-1.5) must fold to -1";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(FC->getNum().get<uint32_t>()), -1);
}

// ---------------------------------------------------------------------------
// 14. i64.trunc_f64_u(4294967295.0) = 4294967295 — unsigned i64 trunc.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case14_TruncU64) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_trunc_u64");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_trunc_u64: i64.trunc_f64_u(4294967295.0) must fold";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I64__const);
  EXPECT_EQ(FC->getNum().get<uint64_t>(), 4294967295ull);
}

// ---------------------------------------------------------------------------
// 15. i64.extend_i32_s(-1) = 0xFFFFFFFFFFFFFFFF — sign extension (sext).
//     https://github.com/llvm/llvm-project/issues/55833
//     https://www.mail-archive.com/llvm-bugs@lists.llvm.org/msg89351.html
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case15_ExtendSignedNeg) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_extend_s_neg");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_extend_s_neg: i64.extend_i32_s(-1) must fold";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I64__const);
  EXPECT_EQ(FC->getNum().get<uint64_t>(), UINT64_MAX); // 0xFFFFFFFFFFFFFFFF
}

// ---------------------------------------------------------------------------
// 16. i64.extend_i32_u(0xFFFFFFFF) = 4294967295 — zero extension.
//     A fold using signed extension would give 0xFFFFFFFFFFFFFFFF.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case16_ExtendUnsignedNeg) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_extend_u_neg");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_extend_u_neg: i64.extend_i32_u(0xFFFFFFFF) must fold";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I64__const);
  EXPECT_EQ(FC->getNum().get<uint64_t>(), 0x00000000FFFFFFFFull);
}

// ---------------------------------------------------------------------------
// 17. i32.trunc_sat_f32_s(NaN) = 0 — saturating trunc does NOT trap.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case17_TruncSatNaN) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_trunc_sat_nan");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_trunc_sat_nan: trunc_sat(NaN) must fold to 0 (no trap)";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I32__const);
  EXPECT_EQ(FC->getNum().get<uint32_t>(), 0u);
}

// ---------------------------------------------------------------------------
// 18. countdown(5) = 0 — bounded recursion must fold.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case18_Countdown) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_countdown");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_countdown: countdown(5) must fold to 0";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I32__const);
  EXPECT_EQ(FC->getNum().get<uint32_t>(), 0u);
}

// ===========================================================================
// Tests derived from V8 and JavaScriptCore bytecode optimizer suites
// ===========================================================================

namespace {
uint64_t getF64Bits(const AST::Instruction &I) {
  double V = I.getNum().get<double>();
  uint64_t B;
  std::memcpy(&B, &V, sizeof(B));
  return B;
}
uint64_t toBits(double V) {
  uint64_t B;
  std::memcpy(&B, &V, sizeof(B));
  return B;
}
} // namespace

// ---------------------------------------------------------------------------
// 19. f64.min(NaN, 42.0) = canonical NaN.
//     WebKit Bug 270262: https://bugs.webkit.org/show_bug.cgi?id=270262
//     JSC test: JSTests/wasm/stress/fp-nan-minmax.js
//     A wrong fold using std::min returns 42.0 (filters NaN).
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case19_F64MinNaN) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_f64_min_nan");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_f64_min_nan: f64.min(NaN, 42.0) must fold to canonical NaN";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::F64__const);
  EXPECT_TRUE(std::isnan(FC->getNum().get<double>()));
}

// ---------------------------------------------------------------------------
// 20. f64.max(NaN, -5.0) = canonical NaN.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case20_F64MaxNaN) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_f64_max_nan");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_f64_max_nan: f64.max(NaN, -5.0) must fold to canonical NaN";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::F64__const);
  EXPECT_TRUE(std::isnan(FC->getNum().get<double>()));
}

// ---------------------------------------------------------------------------
// 21. f64.min(-0.0, +0.0) = -0.0.
//     GCC Bug 116738: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116738
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case21_F64MinNegZero) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_f64_min_neg_zero");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_f64_min_neg_zero: f64.min(-0.0, +0.0) must fold to -0.0";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::F64__const);
  EXPECT_EQ(getF64Bits(*FC), toBits(-0.0));
}

// ---------------------------------------------------------------------------
// 22. f64.max(-0.0, +0.0) = +0.0.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case22_F64MaxNegZero) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_f64_max_neg_zero");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_f64_max_neg_zero: f64.max(-0.0, +0.0) must fold to +0.0";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::F64__const);
  EXPECT_EQ(getF64Bits(*FC), toBits(+0.0));
}

// ---------------------------------------------------------------------------
// 23. algebraic_chain(9) = -5 (not 4).
//     LLVM #96366: https://github.com/llvm/llvm-project/issues/96366
//     Tests (1-x) + (x*1) + (3-x) through an IPCP call site.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case23_AlgebraicChain) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_algebraic_chain");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_algebraic_chain: (1-9)+(9*1)+(3-9) must fold to -5";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(FC->getNum().get<uint32_t>()), -5);
}

// ---------------------------------------------------------------------------
// 24. 0.0 * -1.0 → -0.0 (bit pattern 0x8000000000000000).
//     V8 test: test/mjsunit/minus-zero.js
//     The function returns the i64 reinterpret of the f64 result.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case24_NegZeroMul) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_neg_zero_mul");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_neg_zero_mul: 0.0 * -1.0 must fold to -0.0 (0x8000000000000000)";

  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I64__const);
  EXPECT_EQ(FC->getNum().get<uint64_t>(), UINT64_C(0x8000000000000000));
}

// ---------------------------------------------------------------------------
// 25. memory.size impurity -- call must NOT be folded.
//     Functions that use memory.size observe linear memory state and must be
//     treated as impure by the purity analysis.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case25_MemSizeImpure) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_mem_size_impure");
  EXPECT_TRUE(hasCall(Instrs))
      << "case_mem_size_impure: memory.size makes callee impure, must NOT fold";
}

// ---------------------------------------------------------------------------
// 26. memory.grow impurity -- call must NOT be folded.
//     Functions that use memory.grow mutate linear memory state.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case26_MemGrowImpure) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_mem_grow_impure");
  EXPECT_TRUE(hasCall(Instrs))
      << "case_mem_grow_impure: memory.grow makes callee impure, must NOT fold";
}

// ---------------------------------------------------------------------------
// 27. Transitive impurity -- call must NOT be folded.
//     Even if the caller's OWN instructions are pure, calling an impure callee
//     makes the entire call chain impure.  The purity fixpoint analysis must
//     propagate impurity through the call graph.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case27_TransitiveImpure) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_transitive_impure");
  EXPECT_TRUE(hasCall(Instrs))
      << "case_transitive_impure: transitive impurity through call graph "
         "must prevent folding";
}

// ===========================================================================
// Multi-return tests (cases 28-34)
//
// WASM multi-value proposal allows functions to return multiple values.
// These test IPCP handling of multi-return call site folding.
//
// Derived from:
//   wasm3 #220: multi-return values come back in wrong order
//     https://github.com/wasm3/wasm3/issues/220
//   Wasmtime #2316: incorrect values with >3 return parameters
//     https://github.com/bytecodealliance/wasmtime/issues/2316
//   LLVM #101335: IPSCCP propagates constants to wrong call sites
//     https://github.com/llvm/llvm-project/issues/101335
// ===========================================================================

// ---------------------------------------------------------------------------
// 28. swap(10, 20) = (20, 10) -- value ordering (wasm3 #220 bug class).
//     2 params, 2 returns: exact slot fit (2+1=3 slots, 2 results).
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case28_MultiSwap) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_multi_swap");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_multi_swap: swap(10, 20) should fold";

  // Collect all non-Nop non-End consts in order.
  std::vector<uint32_t> Vals;
  for (const auto &I : Instrs) {
    if (I.getOpCode() == OpCode::I32__const)
      Vals.push_back(I.getNum().get<uint32_t>());
  }
  ASSERT_EQ(Vals.size(), 2u);
  EXPECT_EQ(Vals[0], 20u); // first return = b = 20
  EXPECT_EQ(Vals[1], 10u); // second return = a = 10
}

// ---------------------------------------------------------------------------
// 29. mixed_return(-1) = (-1_i32, -1_i64) -- mixed-type multi-return.
//     1 param, 2 returns (i32, i64): 1+1=2 slots, 2 results -- exact fit.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case29_MultiMixed) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_multi_mixed");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_multi_mixed: mixed_return(-1) should fold";

  // First const: i32 -1.
  std::optional<AST::Instruction> C0;
  std::optional<AST::Instruction> C1;
  for (const auto &I : Instrs) {
    if (I.getOpCode() == OpCode::Nop || I.getOpCode() == OpCode::End)
      continue;
    if (!C0)
      C0 = I;
    else if (!C1)
      C1 = I;
  }
  ASSERT_TRUE(C0.has_value());
  ASSERT_TRUE(C1.has_value());
  EXPECT_EQ(C0->getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(C0->getNum().get<uint32_t>()), -1);
  EXPECT_EQ(C1->getOpCode(), OpCode::I64__const);
  EXPECT_EQ(static_cast<int64_t>(C1->getNum().get<uint64_t>()), -1LL);
}

// ---------------------------------------------------------------------------
// 30. reverse3(1, 2, 3) = (3, 2, 1) -- triple return, exact slot fit.
//     3 params + 1 call = 4 slots, 3 results fit.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case30_MultiTriple) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_multi_triple");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_multi_triple: reverse3(1,2,3) should fold to (3,2,1)";

  std::vector<uint32_t> Vals;
  for (const auto &I : Instrs) {
    if (I.getOpCode() == OpCode::I32__const)
      Vals.push_back(I.getNum().get<uint32_t>());
  }
  ASSERT_EQ(Vals.size(), 3u);
  EXPECT_EQ(Vals[0], 3u);
  EXPECT_EQ(Vals[1], 2u);
  EXPECT_EQ(Vals[2], 1u);
}

// ---------------------------------------------------------------------------
// 31. quad_return(5) -- slot overflow: 4 returns > 1 param + 1 = 2 slots.
//     Must NOT be folded (not enough instruction slots to patch).
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case31_MultiOverflow) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_multi_overflow");
  EXPECT_TRUE(hasCall(Instrs))
      << "case_multi_overflow: NumReturns(4) > NumParams(1)+1 = 2 slots, "
         "must NOT fold";
}

// ---------------------------------------------------------------------------
// 32. void_fn(10, 20) -- void function (0 returns).
//     All slots (2 params + 1 call = 3) become Nops.  The exported caller
//     still has its own i32.const 42 return.
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case32_VoidFn) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_void_fn");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_void_fn: void_fn(10,20) should fold (all slots become Nops)";

  // The only surviving const should be the caller's i32.const 42.
  auto FC = firstConst(Instrs);
  ASSERT_TRUE(FC.has_value());
  EXPECT_EQ(FC->getOpCode(), OpCode::I32__const);
  EXPECT_EQ(FC->getNum().get<uint32_t>(), 42u);
}

// ---------------------------------------------------------------------------
// 33-34. Same function called at two sites with different constant args.
//     swap(1,2) = (2,1) at site A; swap(100,200) = (200,100) at site B.
//     LLVM #101335: IPSCCP propagated constants from one site to another.
//     https://github.com/llvm/llvm-project/issues/101335
// ---------------------------------------------------------------------------

TEST_F(IPCPEdgeCaseTest, Case33_MultiSiteA) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_multi_site_a");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_multi_site_a: swap(1,2) should fold to (2,1)";

  std::vector<uint32_t> Vals;
  for (const auto &I : Instrs)
    if (I.getOpCode() == OpCode::I32__const)
      Vals.push_back(I.getNum().get<uint32_t>());
  ASSERT_EQ(Vals.size(), 2u);
  EXPECT_EQ(Vals[0], 2u);
  EXPECT_EQ(Vals[1], 1u);
}

TEST_F(IPCPEdgeCaseTest, Case34_MultiSiteB) {
  ASSERT_TRUE(SetupOK);
  auto Instrs = getInstrs("case_multi_site_b");
  EXPECT_TRUE(isFolded(Instrs))
      << "case_multi_site_b: swap(100,200) should fold to (200,100)";

  std::vector<uint32_t> Vals;
  for (const auto &I : Instrs)
    if (I.getOpCode() == OpCode::I32__const)
      Vals.push_back(I.getNum().get<uint32_t>());
  ASSERT_EQ(Vals.size(), 2u);
  EXPECT_EQ(Vals[0], 200u);  // NOT 2 from site A!
  EXPECT_EQ(Vals[1], 100u);  // NOT 1 from site A!
}

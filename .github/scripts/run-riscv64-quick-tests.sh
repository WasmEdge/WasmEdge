#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC
#
# RISC-V Quick Test Suite runner.
# Run from a directory that contains build/ (WasmEdge RISC-V64 build tree).
# Used by CI under QEMU and can be run on native RISC-V hardware for verification.
#
# Usage:
#   ./run-riscv64-quick-tests.sh [ARTIFACTS_DIR]
#
#   ARTIFACTS_DIR  Directory containing build/ (default: current directory).
#
# Environment:
#   USE_QEMU  Set to 0 to run natively (e.g. on RISC-V hardware). Default: auto
#             (use QEMU when host arch is not riscv64).
#   TIMEOUT   Per-test timeout in seconds (default: 300).

set -euo pipefail

ARTIFACTS_DIR="${1:-.}"
TIMEOUT="${TIMEOUT:-300}"

# Use QEMU when not on native RISC-V (e.g. CI on x86_64)
if [ "${USE_QEMU:-auto}" = "auto" ]; then
  if [ "$(uname -m)" = "riscv64" ]; then
    USE_QEMU=0
  else
    USE_QEMU=1
  fi
fi

cd "$ARTIFACTS_DIR"

if [ ! -d build/test ] || [ ! -d build/lib ]; then
  echo "Error: build/test or build/lib not found in $ARTIFACTS_DIR" >&2
  exit 1
fi

# =============================================================
# RISC-V Quick Test Suite
# =============================================================
# A focused subset of tests for basic execution verification
# under QEMU user-mode emulation or on native RISC-V. Confirms
# that compilation is sound and fundamental WasmEdge components
# work correctly within a reasonable time.
#
# Tests are categorized into three groups:
#   1. QUICK  - Fast unit/integration tests (enabled)
#   2. HEAVY  - Spec-test-based suites, too slow for QEMU (disabled)
#   3. FLAKY  - Unreliable under QEMU user-mode emulation (disabled)
# =============================================================

# --- Group 1: Quick tests (enabled) ---
# Core utilities, Loader, Validator, Runtime, API, AOT (Blake3).
QUICK_TESTS="
  wasmedgeCommonTests
  wasmedgeErrinfoTests
  expectedTests
  poTests
  spanTests
  wasmedgeLoaderFileMgrTests
  wasmedgeLoaderASTTests
  wasmedgeLoaderSerializerTests
  wasmedgeValidatorSubtypeTests
  wasmedgeMemLimitTests
  wasmedgeExternrefTests
  wasmedgeHostMockTests
  wasmedgeAPIUnitTests
  wasmedgeAOTBlake3Tests
"

# --- Group 2: Time-consuming (disabled) ---
# wasmedgeExecutorCoreTests, wasmedgeAPIVMCoreTests, wasmedgeAPIStepsCoreTests,
# wasmedgeAPIAOTCoreTests, wasmedgeAPIAOTNestedVMTests, wasmedgeLLVMCoreTests,
# wasmedgeAOTCacheTests, wasmedgeMixcallTests, componentTests

# --- Group 3: Expected-to-fail under QEMU (disabled) ---
# wasmedgeThreadTests, wasiTests, wasiSocketTests

run_test() {
  local test_name="$1"
  local test_bin test_dir rel_lib

  test_bin=$(find build/test -type f -executable -name "${test_name}" 2>/dev/null | head -1)
  if [ -z "${test_bin}" ]; then
    echo "SKIP: ${test_name} (binary not found)"
    return 0
  fi

  echo "=== Running: ${test_name} ==="
  test_dir=$(dirname "${test_bin}")
  rel_lib=$(realpath --relative-to="${test_dir}" build/lib)

  if [ "$USE_QEMU" -eq 1 ]; then
    ( cd "${test_dir}" && timeout "$TIMEOUT" qemu-riscv64-static \
      -L /usr/riscv64-linux-gnu \
      -E LD_LIBRARY_PATH="/usr/lib/riscv64-linux-gnu:${rel_lib}" \
      "./${test_name}" )
  else
    ( cd "${test_dir}" && timeout "$TIMEOUT" env LD_LIBRARY_PATH="${rel_lib}" "./${test_name}" )
  fi
}

failed=0
for test_name in ${QUICK_TESTS}; do
  if run_test "$test_name"; then
    echo "PASSED: ${test_name}"
  else
    echo "FAILED: ${test_name}"
    failed=$((failed + 1))
  fi
done

echo "=== Quick Test Suite Complete ==="
if [ "$failed" -gt 0 ]; then
  echo "${failed} test(s) failed"
  exit 1
fi
echo "All quick tests passed"

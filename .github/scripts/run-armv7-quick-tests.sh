#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#
# ARMv7 hard-float test runner for the native AOT linker.
# Run from a directory that contains build/ (WasmEdge ARMv7 build tree).
# Used by CI under QEMU and can be run on native ARMv7 hardware.
#
# Usage:
#   ./run-armv7-quick-tests.sh [ARTIFACTS_DIR]
#
# Environment:
#   USE_QEMU  Set to 0 to run natively. Default: auto (use QEMU when the host
#             is not 32-bit ARM).
#   TIMEOUT   Per-test timeout in seconds (default: 600).

set -euo pipefail

ARTIFACTS_DIR="${1:-.}"
TIMEOUT="${TIMEOUT:-600}"

if [ "${USE_QEMU:-auto}" = "auto" ]; then
  case "$(uname -m)" in
    armv7*) USE_QEMU=0 ;;
    *) USE_QEMU=1 ;;
  esac
fi

cd "$ARTIFACTS_DIR"

TEST_BIN=build/test/llvm/wasmedgeLLVMCoreTests
if [ ! -x "$TEST_BIN" ] || [ ! -d build/lib ] || [ ! -d build/test/spec/testSuites ]; then
  echo "Error: $TEST_BIN, build/lib, or build/test/spec/testSuites not found in $ARTIFACTS_DIR" >&2
  exit 1
fi

run_llvm_tests() {
  local test_dir rel_lib
  test_dir=$(dirname "$TEST_BIN")
  rel_lib=$(realpath --relative-to="$test_dir" build/lib)
  if [ "$USE_QEMU" -eq 1 ]; then
    ( cd "$test_dir" && timeout "$TIMEOUT" qemu-arm-static \
      -L /usr/arm-linux-gnueabihf \
      -E LD_LIBRARY_PATH="/usr/lib/arm-linux-gnueabihf:${rel_lib}" \
      ./wasmedgeLLVMCoreTests "$@" )
  else
    ( cd "$test_dir" && timeout "$TIMEOUT" env LD_LIBRARY_PATH="${rel_lib}" \
      ./wasmedgeLLVMCoreTests "$@" )
  fi
}

failed=0

# Run a filter that must execute at least one test and skip none. A skip
# means the host check rejected this platform, and zero tests means the
# filter no longer matches a renamed test.
run_required() {
  local name="$1" filter="$2" output
  output=$(mktemp)
  if run_llvm_tests --gtest_filter="$filter" | tee "$output" &&
    grep -Eq '^\[  PASSED  \] [1-9][0-9]* tests?\.' "$output" &&
    ! grep -q '\[  SKIPPED \]' "$output"; then
    echo "PASSED: ${name}"
  else
    echo "FAILED: ${name}"
    failed=$((failed + 1))
  fi
  rm -f "$output"
}

# The execution test compiles, links, loads, and runs a module that calls
# the embedded ARM helpers.
run_required "ARM helper execution test" \
  'ARMRuntimeLibcallsIntegrationTest.CompilerAndCodeGenExecuteNumericWasmOnARMHost'
run_required "native linker AOT tests" 'AOTLinker.NativeLinker*'

output=$(mktemp)
if run_llvm_tests \
    --gtest_filter='ARMRuntimeLibcalls*:ARMRelocationTest.*:LinkerHostTest.*' |
    tee "$output" &&
  grep -Eq '^\[  PASSED  \] [1-9][0-9]* tests?\.' "$output"; then
  echo "PASSED: ARM linker subset"
else
  echo "FAILED: ARM linker subset"
  failed=$((failed + 1))
fi
rm -f "$output"

if [ "$failed" -gt 0 ]; then
  echo "${failed} test group(s) failed"
  exit 1
fi
echo "All ARMv7 tests passed"

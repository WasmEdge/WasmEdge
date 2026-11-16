#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#
# ARMv7 test suite runner.
# Run from a directory that contains build/ (WasmEdge ARMv7 build tree).
# Used by CI under QEMU and can be run on native ARMv7 hardware for verification.
#
# Usage:
#   ./run-armv7-tests.sh [ARTIFACTS_DIR]
#
#   ARTIFACTS_DIR  Directory containing build/ (default: current directory).
#
# Environment:
#   USE_QEMU  Set to 0 to run natively (e.g. on ARMv7 hardware). Default: auto
#             (use QEMU when host arch is not armv7l).
#   SYSROOT   Sysroot for QEMU (default: /usr/arm-linux-gnueabihf).
#   TIMEOUT   Per-test timeout in seconds (default: 3600).

set -euo pipefail

ARTIFACTS_DIR="${1:-.}"
TIMEOUT="${TIMEOUT:-3600}"
SYSROOT="${SYSROOT:-/usr/arm-linux-gnueabihf}"

# Use QEMU when not on native ARMv7 (e.g. CI on x86_64)
if [ "${USE_QEMU:-auto}" = "auto" ]; then
  if [ "$(uname -m)" = "armv7l" ]; then
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
# ARMv7 Test Suite
# =============================================================
# ARMv7 is a 32-bit host, so the reference heap runs in chunk mode. The
# spec test suite runs in the interpreter to cover the chunk mode decode on
# a 32-bit target.
#
# Compiled code support on ARMv7 is limited. wasmedgeLLVMCoreTests stays
# disabled because compiled code on a 32-bit host fails the linear memory and
# reference cast suites, also without the reference heap.
#
# Tests that are unreliable under QEMU user-mode emulation stay disabled:
# wasmedgeThreadTests, wasiTests, wasiSocketTests.
# =============================================================
TESTS="
  wasmedgeCommonTests
  wasmedgeErrinfoTests
  expectedTests
  poTests
  spanTests
  wasmedgeLoaderFileMgrTests
  wasmedgeLoaderASTTests
  wasmedgeLoaderSerializerTests
  wasmedgeValidatorRegressionTests
  wasmedgeRuntimeInstanceTests
  wasmedgeExternrefTests
  wasmedgeHostMockTests
  wasmedgeAPIUnitTests
  wasmedgeAOTBlake3Tests
  wasmedgeExecutorCoreTests
"

run_test() {
  local test_name="$1"
  local test_bin test_dir rel_lib

  test_bin=$(find build/test -type f -executable -name "${test_name}" 2>/dev/null | head -1)
  if [ -z "${test_bin}" ]; then
    echo "MISSING: ${test_name} (binary not found)"
    return 1
  fi

  echo "=== Running: ${test_name} ==="
  test_dir=$(dirname "${test_bin}")
  rel_lib=$(realpath --relative-to="${test_dir}" build/lib)

  if [ "$USE_QEMU" -eq 1 ]; then
    ( cd "${test_dir}" && timeout "$TIMEOUT" qemu-arm-static \
      -L "${SYSROOT}" \
      -E LD_LIBRARY_PATH="/usr/lib/arm-linux-gnueabihf:${rel_lib}" \
      "./${test_name}" )
  else
    ( cd "${test_dir}" && timeout "$TIMEOUT" env LD_LIBRARY_PATH="${rel_lib}" "./${test_name}" )
  fi
}

failed=0
for test_name in ${TESTS}; do
  if run_test "$test_name"; then
    echo "PASSED: ${test_name}"
  else
    echo "FAILED: ${test_name}"
    failed=$((failed + 1))
  fi
done

echo "=== Test Suite Complete ==="
if [ "$failed" -gt 0 ]; then
  echo "${failed} test(s) failed"
  exit 1
fi
echo "All tests passed"

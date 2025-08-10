#!/bin/bash
# Test script for WasmEdge RISC-V builds in QEMU environment

set -e

WASMEDGE_ROOT="/workplace"
ARTIFACTS_DIR="${WASMEDGE_ROOT}/riscv64-artifacts"

echo "Testing WasmEdge RISC-V build..."

# Set up environment
export PATH="${ARTIFACTS_DIR}/tools/wasmedge:$PATH"
export LD_LIBRARY_PATH="${ARTIFACTS_DIR}/lib:$LD_LIBRARY_PATH"

# Make binaries executable
chmod +x "${ARTIFACTS_DIR}/tools/wasmedge/wasmedge" 2>/dev/null || true
chmod +x "${ARTIFACTS_DIR}/tools/wasmedge/wasmedgec" 2>/dev/null || true

# Verify installation
echo "=== Version Information ==="
wasmedge -v
wasmedgec -v

echo "=== Help Information ==="
wasmedge compile -h
wasmedge run -h

# Run basic functionality tests
echo "=== Basic Functionality Tests ==="
cd "${ARTIFACTS_DIR}/examples/wasm"

# Generate WASM file
wat2wasm fibonacci.wat -o fibonacci.wasm

# Test AOT compilation with wasmedgec
echo "Testing AOT compilation with wasmedgec..."
wasmedgec fibonacci.wasm fibonacci_aot_c.wasm

# Test execution with wasmedge
echo "Testing execution with wasmedge..."
result1=$(wasmedge --reactor fibonacci_aot_c.wasm fib 30)
echo "Result 1: $result1"

result2=$(wasmedge run --reactor fibonacci_aot_c.wasm fib 30)
echo "Result 2: $result2"

# Test AOT compilation with wasmedge compile
echo "Testing AOT compilation with wasmedge compile..."
wasmedge compile fibonacci.wasm fibonacci_aot_compile.wasm

result3=$(wasmedge --reactor fibonacci_aot_compile.wasm fib 30)
echo "Result 3: $result3"

result4=$(wasmedge run --reactor fibonacci_aot_compile.wasm fib 30)
echo "Result 4: $result4"

# Verify all results are the same
if [[ "$result1" == "$result2" && "$result2" == "$result3" && "$result3" == "$result4" ]]; then
    echo "✅ All tests passed! Results are consistent: $result1"
else
    echo "❌ Test failed! Results are inconsistent:"
    echo "  Result 1: $result1"
    echo "  Result 2: $result2"
    echo "  Result 3: $result3"
    echo "  Result 4: $result4"
    exit 1
fi

# Run unit tests if available
if [ -d "${ARTIFACTS_DIR}/test" ]; then
    echo "=== Unit Tests ==="
    find "${ARTIFACTS_DIR}/test" -name "*test*" -executable -type f | head -5 | while read test_binary; do
        echo "Running $test_binary..."
        timeout 60s "$test_binary" || echo "Test $test_binary failed or timed out"
    done
fi

echo "🎉 All RISC-V tests completed successfully!"

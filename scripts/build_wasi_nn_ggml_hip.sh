#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=build-wasi-nn-ggml-hip
mkdir -p "${BUILD_DIR}"

cmake -B "${BUILD_DIR}" -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DWASMEDGE_BUILD_PLUGINS=ON \
  -DWASMEDGE_PLUGIN_WASI_NN_BACKEND=ggml \
  -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA=ON \
  -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP=ON \
  ${ARCH_ARG:+-DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP_ARCH="${ARCH_ARG}"} \
  -DWASMEDGE_BUILD_TESTS=OFF

cmake ${CMAKE_ARGS} ..    
if ! cmake --build "${BUILD_DIR}" -j "$(nproc)" --target wasmedgePluginWasiNN; then
  echo "Build failed."
  exit 1
fi

# locate built plugin (example path)
PLUGIN_LIB=$(find "${BUILD_DIR}" -name "libwasmedgePluginWasiNN*.so" | head -n1)
if [ -z "$PLUGIN_LIB" ]; then
  echo "Built plugin not found."
  exit 2
fi

# package artifact
ARTIFACTNAME="wasmedge-wasi-nn-ggml-hip-$(date +%Y%m%d).tar.gz"
tar -czf "${ARTIFACTNAME}" -C "$(dirname "$PLUGIN_LIB")" "$(basename "$PLUGIN_LIB")"
echo "Built and packed: ${ARTIFACTNAME}" 

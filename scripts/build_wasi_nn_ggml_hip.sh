#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=build-wasi-nn-ggml-hip
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# pass any needed ROCm arch targets (optional)
# Example: -D CMAKE_HIP_ARCHITECTURES=gfx1032
CMAKE_ARGS="-DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP=ON -DWASMEDGE_PLUGIN_WASI_NN_BACKEND=ggml -DWASMEDGE_BUILD_PLUGINS=ON -DCMAKE_BUILD_TYPE=Release"

cmake ${CMAKE_ARGS} ..    
cmake --build . --target wasmedgePluginWasiNN -- -j$(nproc)

# locate built plugin (example path)
PLUGIN_LIB=$(find . -name "libwasmedgePluginWasiNN*.so" | head -n1)
if [ -z "$PLUGIN_LIB" ]; then
  echo "Built plugin not found."
  exit 2
fi

# package artifact
ARTIFACTNAME="wasmedge-wasi-nn-ggml-hip-$(date +%Y%m%d).tar.gz"
tar -czf ../${ARTIFACTNAME} "$PLUGIN_LIB"
echo "Built and packed: ../${ARTIFACTNAME}"

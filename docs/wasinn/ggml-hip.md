# WASI-NN GGML with HIP Backend

This document describes how to build and use the WasmEdge WASI-NN plugin with HIP backend support for AMD GPUs.

## Overview

The HIP backend enables the WASI-NN GGML plugin to leverage AMD GPUs for accelerated inference using the llama.cpp library. This provides significant performance improvements when running large language models on AMD hardware.

## Prerequisites

- AMD GPU with ROCm support
- Linux operating system (Ubuntu 20.04/22.04, RHEL/CentOS 8+)
- CMake 3.18 or later
- ROCm/HIP development environment

## Installation

### 1. Install ROCm/HIP

Follow the official ROCm installation guide: https://rocm.docs.amd.com/

**Ubuntu 22.04:**
```bash
sudo apt update
wget -q -O - https://repo.radeon.com/rocm/rocm.gpg.key | sudo apt-key add -
echo "deb [arch=amd64] https://repo.radeon.com/rocm/apt/5.7/ ubuntu main" | sudo tee /etc/apt/sources.list.d/rocm.list
sudo apt update
sudo apt install -y rocm-dev rocm-utils hipblas hip-dev
```

### 2. Setup Environment

To set up your environment, you can run the following commands to check for `hipcc` and set the `ROCM_PATH` environment variable.

```bash
#!/usr/bin/env bash
# This script is for manual environment setup only. It is not used in CI workflows.
set -euo pipefail

echo "Checking HIP/ROCm environment..."

if command -v hipcc >/dev/null 2>&1; then
  echo "hipcc found: $(which hipcc)"
else
  echo "hipcc not found."
  echo "Please install ROCm/HIP from https://rocm.docs.amd.com/"
  echo "Example (Ubuntu 22.04):"
  echo "  sudo apt update && sudo apt install -y rocm-dev rocm-utils hipblas"
  exit 1
fi

if command -v hipconfig >/dev/null 2>&1; then
  hipconfig --full || true
fi

# Set ROCM_PATH if typical location exists
if [ -d /opt/rocm ]; then
  export ROCM_PATH=/opt/rocm
  echo "ROCM_PATH set to /opt/rocm"
fi
```

Alternatively, you can set the environment variables manually:

```bash
export ROCM_PATH=/opt/rocm
export PATH=$ROCM_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ROCM_PATH/lib:$LD_LIBRARY_PATH
```

### 3. Build WasmEdge with HIP Support

You can use the following script to build the WasmEdge plugin with HIP support. This script allows you to pass target architectures to optimize the build.

```bash
#!/usr/bin/env bash
# This script is for manual builds only. It is not used in CI workflows.
set -euo pipefail

ARCH_ARG=""
if [[ $# -gt 0 && "$1" == "--arch" ]]; then
  ARCH_ARG="$2"
fi

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
```

To run the build, save the script above as `build_hip.sh`, make it executable (`chmod +x build_hip.sh`), and then run it, optionally passing your GPU architecture:

```bash
./build_hip.sh --arch gfx90a;gfx1030
```

Or build manually:
```bash
mkdir build-hip && cd build-hip
cmake .. \
  -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP=ON \
  -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP_ARCH=gfx90a;gfx1030 \
  -DWASMEDGE_PLUGIN_WASI_NN_BACKEND=ggml \
  -DWASMEDGE_BUILD_PLUGINS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build . --target wasmedgePluginWasiNN -j$(nproc)
```

## Configuration Options

- `WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP`: Enable HIP backend (default: OFF)
- `WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP_ARCH`: Semicolon separated HIP GPU architectures (e.g., `gfx90a;gfx1030`). When set, forwarded to `CMAKE_HIP_ARCHITECTURES`.
- `CMAKE_HIP_ARCHITECTURES`: (Advanced) Direct CMake variable; normally you can just use the plugin-specific variable above.

## Usage

Once built with HIP support, the WASI-NN plugin will automatically detect and use AMD GPUs when available. No additional configuration is needed in your WebAssembly applications.

The plugin will prefer HIP acceleration over CPU execution when:
1. AMD GPU hardware is detected
2. ROCm drivers are properly installed
3. Sufficient GPU memory is available

## Testing

Verify your setup:
```bash
# Check HIP installation
hipconfig --full

# Check GPU detection
rocm-smi --showproductname

# Test basic HIP functionality
echo '#include <hip/hip_runtime.h>
#include <iostream>
int main() {
    int deviceCount;
    hipGetDeviceCount(&deviceCount);
    std::cout << "HIP devices: " << deviceCount << std::endl;
    return 0;
}' > test.cpp && hipcc test.cpp -o test && ./test
```

## Troubleshooting

### Common Issues

1. **hipcc not found**: Ensure ROCm is installed and `ROCM_PATH` is set
2. **CMake HIP detection / arch detection fails**: Provide an explicit architecture list via `-DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP_ARCH=gfx90a` (replace with your GPU) or ensure ROCm supports your hardware
3. **No AMD GPU detected**: Verify hardware support with `rocm-smi`
4. **Out of memory errors**: Reduce model size or use quantized models

### Performance Tips

- Use FP16 models for better memory efficiency
- Ensure adequate GPU memory (8GB+ recommended for 7B models)
- Monitor GPU utilization with `rocm-smi`

## Limitations

- HIP backend requires AMD GPUs with ROCm support
- Some model types may fall back to CPU execution
- Performance varies by GPU architecture and model size

## Support

For issues related to:
- ROCm installation: https://rocm.docs.amd.com/
- WasmEdge: https://github.com/WasmEdge/WasmEdge/issues
- llama.cpp HIP: https://github.com/ggml-org/llama.cpp/issues

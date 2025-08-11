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

Use the provided setup script:
```bash
./scripts/setup_hip_env.sh
```

Or manually set environment variables:
```bash
export ROCM_PATH=/opt/rocm
export PATH=$ROCM_PATH/bin:$PATH
export LD_LIBRARY_PATH=$ROCM_PATH/lib:$LD_LIBRARY_PATH
```

### 3. Build WasmEdge with HIP Support

Use the build script:
```bash
./scripts/build_wasi_nn_ggml_hip.sh
```

Or build manually:
```bash
mkdir build-hip && cd build-hip
cmake .. \
  -DWASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP=ON \
  -DWASMEDGE_PLUGIN_WASI_NN_BACKEND=ggml \
  -DWASMEDGE_BUILD_PLUGINS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build . --target wasmedgePluginWasiNN -j$(nproc)
```

## Configuration Options

- `WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP`: Enable HIP backend (default: OFF)
- `CMAKE_HIP_ARCHITECTURES`: Specify target GPU architectures (e.g., "gfx900;gfx906;gfx908")

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
2. **CMake HIP detection fails**: Check that hip-dev package is installed
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

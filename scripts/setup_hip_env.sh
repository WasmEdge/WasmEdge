#!/usr/bin/env bash
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

echo "Done. Now you can run: ./scripts/build_wasi_nn_ggml_hip.sh"

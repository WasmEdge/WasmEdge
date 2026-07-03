#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CACHE_DIR="${ROOT}/test/plugins/wasi_nn/fixtures_cache"
MODEL="${CACHE_DIR}/ggml-model-i2_s.gguf"
URL="https://huggingface.co/microsoft/bitnet-b1.58-2B-4T-gguf/resolve/main/ggml-model-i2_s.gguf"
EXPECTED_MD5="65cb04366e4d02ccd78b4b7b48c84b3b"

mkdir -p "${CACHE_DIR}"

if [[ -f "${MODEL}" ]] && md5sum -b "${MODEL}" | awk '{print $1}' | grep -qx "${EXPECTED_MD5}"; then
  echo "fixture already cached: ${MODEL}"
  exit 0
fi

echo "downloading BitNet i2_s fixture to ${MODEL}"

if command -v aria2c >/dev/null 2>&1; then
  aria2c -x16 -s16 -k1M --file-allocation=none -d "${CACHE_DIR}" -o "ggml-model-i2_s.gguf" "${URL}"
elif command -v wget >/dev/null 2>&1; then
  wget --http1.1 --continue --progress=dot:giga -O "${MODEL}.part" "${URL}"
  mv "${MODEL}.part" "${MODEL}"
else
  curl -L --http1.1 --retry 5 --retry-delay 2 -C - -o "${MODEL}.part" "${URL}"
  mv "${MODEL}.part" "${MODEL}"
fi

ACTUAL_MD5="$(md5sum -b "${MODEL}" | awk '{print $1}')"
if [[ "${ACTUAL_MD5}" != "${EXPECTED_MD5}" ]]; then
  echo "md5 mismatch: expected ${EXPECTED_MD5}, got ${ACTUAL_MD5}" >&2
  exit 1
fi

echo "download complete: ${MODEL}"

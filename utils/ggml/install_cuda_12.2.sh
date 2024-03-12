#!/bin/bash

set -ex

# Download the CUDA GPG key package.
CUDA_KEYRING_PACKAGE="cuda-keyring_1.1-1_all.deb"
CUDA_KEYRING_URL="https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/${CUDA_KEYRING_PACKAGE}"
dpkg -i -y "${CUDA_KEYRING_PACKAGE}"
rm -f "${CUDA_KEYRING_PACKAGE}"

# Install the CUDA Toolkit 12.2.
apt-get update
apt-get -y install cuda-toolkit-12-2

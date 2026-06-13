#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

set -e

apt-get update
apt-get -y install python3 python3-dev

# Use pinned pip
python3 -m venv chattts_venv
source chattts_venv/bin/activate
pip install --require-hashes -r /dev/stdin <<'EOF'
pip==24.3.1 --hash=sha256:3790624780082365f47549d032f3770eeb2b1e8bd1f7b2e02dace1afa361b4ed
EOF

# Install PyTorch CPU version to save space
pip --python /usr/bin/python3 install --break-system-packages --index-url https://download.pytorch.org/whl/cpu 'torch<=2.6.0' 'torchaudio<=2.6.0'
pip --python /usr/bin/python3 install --break-system-packages chattts==0.2.4, transformers==4.46.3

# Remove wheel cache
pip --python /usr/bin/python3 cache purge

# Clean up
deactivate
rm -rf chattts_venv

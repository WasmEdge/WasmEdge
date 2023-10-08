#!/usr/bin/env bash

wget -O ffmpeg.zip https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n6.0.zip

unzip ffmpeg.zip

cd FFmpeg-n6.0
./configure --prefix=/usr/local --enable-gpl --enable-nonfree --enable-shared --disable-static
make install

# Configure
cmake -GNinja ..
# Build
cmake --build .
# Install to system
cmake --install .

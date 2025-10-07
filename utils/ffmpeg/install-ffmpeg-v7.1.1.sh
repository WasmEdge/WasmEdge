#!/usr/bin/env bash
set -e

curl -sL https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n7.1.1.zip -o ffmpeg.zip

unzip ffmpeg.zip

mkdir -p FFmpeg-n7.1.1/output
cd FFmpeg-n7.1.1
./configure --prefix=$(pwd)/output --enable-gpl --enable-nonfree --enable-shared --disable-static
make && make install
cd ..

rm -rf ffmpeg.zip

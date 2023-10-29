#!/usr/bin/env bash

curl -sL https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n6.0.zip -o ffmpeg.zip

unzip ffmpeg.zip

cd FFmpeg-n6.0
./configure --prefix=/usr/local --enable-gpl --enable-nonfree --enable-shared --disable-static
make install
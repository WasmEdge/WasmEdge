#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC

# The below video used is sourced from an ffmpeg-libav-tutorial repository.
# Source: https://github.com/leandromoreira/ffmpeg-libav-tutorial/blob/master/LICENSE.
TODIR=$1
SAMPLE_VIDEO=https://raw.githubusercontent.com/Hrushi20/rust-ffmpeg/master/assets/bunny.mp4
if [[ $# -eq 0 ]]; then
    TODIR=.
fi
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi

if [ ! -f $TODIR/sample_video.mp4 ]; then
  curl -sL $SAMPLE_VIDEO -o $TODIR/sample_video.mp4
  cp $TODIR/sample_video.mp4 $TODIR/dummy.mp4 # Dummy file to manipulate and run tests on file.
fi

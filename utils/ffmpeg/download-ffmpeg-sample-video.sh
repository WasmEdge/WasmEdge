#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

TODIR=$1
SAMPLE_VIDEO=https://raw.githubusercontent.com/Hrushi20/ffmpeg-rust/main/assets/small_bunny_1080p_60fps.mp4
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
fi

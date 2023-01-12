#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

TODIR=$1
if [[ $# -eq 0 ]]; then
    TODIR=.
fi
FIXTURE=https://github.com/intel/openvino-rs/raw/v0.3.3/crates/openvino/tests/fixtures/mobilenet/
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi

if [ ! -f $TODIR/mobilenet.bin ]; then
    curl -sL $FIXTURE/mobilenet.bin -o $TODIR/mobilenet.bin
fi
if [ ! -f $TODIR/mobilenet.xml ]; then
    curl -sL $FIXTURE/mobilenet.xml -o $TODIR/mobilenet.xml
fi
if [ ! -f $TODIR/tensor-1x224x224x3-f32.bgr ]; then
    curl -sL $FIXTURE/tensor-1x224x224x3-f32.bgr -o $TODIR/tensor-1x224x224x3-f32.bgr
fi

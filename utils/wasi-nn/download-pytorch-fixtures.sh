#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

TODIR=$1
if [[ $# -eq 0 ]]; then
    TODIR=.
fi
FIXTURE=https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/pytorch-mobilenet-image/
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi

if [ ! -f $TODIR/mobilenet.pt ]; then
    curl -sL $FIXTURE/mobilenet.pt -o $TODIR/mobilenet.pt
fi
if [ ! -f $TODIR/image-1x3x224x224.rgb ]; then
    curl -sL $FIXTURE/image-1x3x224x224.rgb -o $TODIR/image-1x3x224x224.rgb
fi

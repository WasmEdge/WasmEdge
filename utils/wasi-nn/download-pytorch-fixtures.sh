#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

DOWNLOAD_TO=$1
FIXTURE=https://github.com/second-state/WasmEdge-WASINN-examples/raw/master/pytorch-mobilenet-image/
if [ ! -f $DOWNLOAD_TO/mobilenet.pt ]; then
    wget -q --no-clobber --directory-prefix=$DOWNLOAD_TO $FIXTURE/mobilenet.pt
fi

if [ ! -f $DOWNLOAD_TO/image-1x3x224x224.rgb ]; then
    wget -q --no-clobber --directory-prefix=$DOWNLOAD_TO $FIXTURE/image-1x3x224x224.rgb
fi

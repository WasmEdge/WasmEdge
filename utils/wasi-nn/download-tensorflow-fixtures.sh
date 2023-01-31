#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

TODIR=$1
if [[ $# -eq 0 ]]; then
    TODIR=.
fi
FIXTURE=https://raw.githubusercontent.com/gusye1234/WasmEdge-WASINN-examples/tf-demo/tf-mobilenet-image
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi

if [ ! -f $TODIR/saved_model.pb ]; then
    curl -sL $FIXTURE/saved_model.pb -o $TODIR/saved_model.pb
fi
if [ ! -f $TODIR/birdX224X224X3.rgb ]; then
    curl -sL $FIXTURE/birdX224X224X3.rgb -o $TODIR/birdX224X224X3.rgb
fi
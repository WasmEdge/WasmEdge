#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2023 Second State INC

TODIR=$1
if [[ $# -eq 0 ]]; then
    TODIR=.
fi
MODEL=orca-mini-3b.ggmlv3.q4_0.bin
FIXTURE=https://huggingface.co/TheBloke/orca_mini_3B-GGML/resolve/main/$MODEL
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi

if [ ! -f $TODIR/$MODEL ]; then
    curl -sL $FIXTURE -o $TODIR/$MODEL
fi

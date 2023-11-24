#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2023 Second State INC

TODIR=$1
if [[ $# -eq 0 ]]; then
    TODIR=.
fi
MODEL=orca_mini.gguf
FIXTURE=https://huggingface.co/TheBloke/orca_mini_v3_7B-GGUF/resolve/main/orca_mini_v3_7b.Q2_K.gguf
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi

if [ ! -f $TODIR/$MODEL ]; then
    curl -sL $FIXTURE -o $TODIR/$MODEL
fi

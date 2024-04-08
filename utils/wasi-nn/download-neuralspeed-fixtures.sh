#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2023 Second State INC

TODIR=$1
if [[ $# -eq 0 ]]; then
    TODIR=.
fi
MODEL=llama-2-7b-chat.Q4_0.gguf
FIXTURE=https://huggingface.co/TheBloke/Llama-2-7B-Chat-GGUF/resolve/main/llama-2-7b-chat.Q4_0.gguf
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi

if [ ! -f $TODIR/$MODEL ]; then
    curl -sL $FIXTURE -o $TODIR/$MODEL
fi

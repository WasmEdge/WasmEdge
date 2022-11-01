#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

TODIR=$1
if [[ $# -eq 0 ]]; then
    TODIR=.
fi
FIXTURE=https://raw.githubusercontent.com/gusye1234/WasmEdge-WASINN-examples/demo-tflite-image/tflite-birds_v1-image
if [ ! -d $TODIR ]; then
    mkdir $TODIR
fi

if [ ! -f $TODIR/lite-model_aiy_vision_classifier_birds_V1_3.tflite ]; then
    curl -sL $FIXTURE/lite-model_aiy_vision_classifier_birds_V1_3.tflite -o $TODIR/lite-model_aiy_vision_classifier_birds_V1_3.tflite
fi
if [ ! -f $TODIR/birdx224x224x3.rgb ]; then
    curl -sL $FIXTURE/birdx224x224x3.rgb -o $TODIR/birdx224x224x3.rgb
fi

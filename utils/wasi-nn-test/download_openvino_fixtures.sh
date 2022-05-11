#!/usr/bin/env bash

DOWNLOAD_TO=$1
FIXTURE=https://github.com/intel/openvino-rs/raw/v0.3.3/crates/openvino/tests/fixtures/mobilenet/

if [ ! -f $DOWNLOAD_TO/mobilenet.bin ]; then
    wget -q --no-clobber --directory-prefix=$DOWNLOAD_TO $FIXTURE/mobilenet.bin
fi
if [ ! -f $DOWNLOAD_TO/mobilenet.xml ]; then
    wget -q --no-clobber --directory-prefix=$DOWNLOAD_TO $FIXTURE/mobilenet.xml
fi
if [ ! -f $DOWNLOAD_TO/tensor.bgr ]; then
    wget -q --no-clobber $FIXTURE/tensor-1x224x224x3-f32.bgr --output-document=$DOWNLOAD_TO/tensor.bgr
fi

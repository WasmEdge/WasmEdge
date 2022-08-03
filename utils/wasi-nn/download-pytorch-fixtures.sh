#!/usr/bin/env bash

DOWNLOAD_TO=$1
FIXTURE=https://github.com/gusye1234/torchscript_fixtures/raw/main/mobilenet

if [ ! -f $DOWNLOAD_TO/mobilenet.pt ]; then
    wget -q --no-clobber --directory-prefix=$DOWNLOAD_TO $FIXTURE/mobilenet.pt
fi

if [ ! -f $DOWNLOAD_TO/image-1-3-244-244.rgb ]; then
    wget -q --no-clobber --directory-prefix=$DOWNLOAD_TO $FIXTURE/image-1-3-244-244.rgb
fi

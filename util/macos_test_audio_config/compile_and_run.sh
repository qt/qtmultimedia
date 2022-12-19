#! /bin/sh
# Copyright (C) 2019 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

BUILD_DIR="__build__"
OUTPUT_FILE="test_audio_config_res.txt"

mkdir $BUILD_DIR
cd $BUILD_DIR
cmake ..
make -j
cd ..

rm -f $OUTPUT_FILE
./$BUILD_DIR/test_audio_config "$@" >> $OUTPUT_FILE
cat $OUTPUT_FILE
echo "\nThe result has been written to file $OUTPUT_FILE\n"

rm -rf $BUILD_DIR

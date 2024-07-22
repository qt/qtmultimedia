#!/bin/bash
# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generate some simple test data.  Uses "sox".

endian=""
endian_extn=""

for channel in 1 2; do
    if [ $channel -eq 1 ]; then
        endian="little"
        endian_extn="le"
    fi

    if [ $channel -eq 2 ]; then
        endian="big"
        endian_extn="be"
    fi
    for samplerate in 44100 8000; do
        for samplebits in 8 16 24 32; do
            if [ $samplebits -ne 8 ]; then
                sox -n --endian "${endian}" -c ${channel} -b ${samplebits} -r ${samplerate} isawav_${channel}_${samplebits}_${samplerate}_${endian_extn}.wav synth 0.25 sine 220
            else
                sox -n -c ${channel} -b ${samplebits} -r ${samplerate} isawav_${channel}_${samplebits}_${samplerate}.wav synth 0.25 sine 220
            fi
        done
        sox -n --endian "${endian}" -c ${channel} -e float -b 32 -r ${samplerate} isawav_${channel}_f32_${samplerate}_${endian_extn}.wav synth 0.25 sine 220
    done
done

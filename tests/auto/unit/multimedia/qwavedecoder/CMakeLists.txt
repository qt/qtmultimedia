# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwavedecoder Test:
#####################################################################

qt_internal_add_test(tst_qwavedecoder
    SOURCES
        tst_qwavedecoder.cpp
        data/gendata.sh
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::Network
        Qt::MultimediaTestLibPrivate
    TESTDATA
        data/corrupt_datadesc_1_16_8000.le.wav
        data/corrupt_fmtdesc_1_16_8000.le.wav
        data/corrupt_fmtstring_1_16_8000.le.wav
        data/empty.wav
        data/gendata.sh
        data/isawav_1_16_44100_le.wav
        data/isawav_1_16_44100_le_2.wav
        data/isawav_1_16_8000_le.wav
        data/isawav_1_24_44100_le.wav
        data/isawav_1_24_8000_le.wav
        data/isawav_1_32_44100_le.wav
        data/isawav_1_32_8000_le.wav
        data/isawav_1_8_44100.wav
        data/isawav_1_8_8000.wav
        data/isawav_1_8_8000_even_bext.wav
        data/isawav_1_8_8000_odd_bext.wav
        data/isawav_1_f32_44100_le.wav
        data/isawav_1_f32_8000_le.wav
        data/isawav_2_16_44100_be.wav
        data/isawav_2_16_8000_be.wav
        data/isawav_2_24_44100_be.wav
        data/isawav_2_24_8000_be.wav
        data/isawav_2_32_44100_be.wav
        data/isawav_2_32_8000_be.wav
        data/isawav_2_8_44100.wav
        data/isawav_2_8_8000.wav
        data/isawav_2_f32_44100_be.wav
        data/isawav_2_f32_8000_be.wav
        data/notawav.wav
        data/nosampledata.wav
        data/onebyte.wav
)

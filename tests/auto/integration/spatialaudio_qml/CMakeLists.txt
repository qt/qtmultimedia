# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_spatialsound_qml
    QMLTEST
    SOURCES
        tst_qml.cpp
        qml/tst_spatialsound.qml
        qml/tst_ambientsound.qml
        qml/tst_audioengine.qml
        qml/tst_audiolistener.qml
        qml/tst_audioroom.qml
    LIBRARIES
        Qt6::Quick
        Qt6::QuickTest
    TESTDATA
        "qml/test.wav"
)

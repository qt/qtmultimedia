# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_multimedia_qml
    QMLTEST
    SOURCES
        tst_qml.cpp
    LIBRARIES
        Qt6::Quick
        Qt6::Multimedia
        Qt6::QuickTest
)

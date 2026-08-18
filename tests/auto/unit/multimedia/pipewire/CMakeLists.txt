# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_multimedia_add_test(tst_qpipewiresupport
    SOURCES
        tst_qpipewiresupport.cpp
    LIBRARIES
        Qt::MultimediaPrivate
        PipeWire::PipeWire
)

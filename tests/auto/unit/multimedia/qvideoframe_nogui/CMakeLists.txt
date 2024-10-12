# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qvideoframe_nogui
    SOURCES
        tst_qvideoframe_nogui.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)

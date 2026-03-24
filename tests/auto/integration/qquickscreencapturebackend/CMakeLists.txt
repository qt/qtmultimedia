# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qquickscreencapturebackend Test:
#####################################################################

qt_internal_add_test(tst_qquickscreencapturebackend
    SOURCES
        tst_qquickscreencapturebackend.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        Qt::QuickPrivate
)

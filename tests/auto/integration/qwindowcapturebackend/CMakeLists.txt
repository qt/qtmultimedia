# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qwindowcapturebackend
    SOURCES
        tst_qwindowcapturebackend.cpp
        widget.h
        widget.cpp
        grabber.h
        grabber.cpp
        fixture.h
        fixture.cpp
    LIBRARIES
        Qt::Multimedia
        Qt::Gui
        Qt::Widgets
        Qt::MultimediaWidgets
)



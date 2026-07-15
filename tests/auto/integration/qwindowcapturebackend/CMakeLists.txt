# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_multimedia_add_test(tst_qwindowcapturebackend
    NEEDS_MEDIA_BACKEND_PLUGINS
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
        Qt::MultimediaPrivate
        Qt::Gui
        Qt::Widgets
        Qt::MultimediaWidgets
        Qt::MultimediaTestLibPrivate
)



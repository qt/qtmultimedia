# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qvideowidget Test:
#####################################################################

qt_internal_add_test(tst_qvideowidget
    SOURCES
        tst_qvideowidget.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
        Qt::MockMultimediaPlugin
        Qt::MultimediaTestLibPrivate
)

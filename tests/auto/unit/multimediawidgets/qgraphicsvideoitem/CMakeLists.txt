# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qgraphicsvideoitem.pro.

#####################################################################
## tst_qgraphicsvideoitem Test:
#####################################################################

qt_internal_add_test(tst_qgraphicsvideoitem
    SOURCES
        tst_qgraphicsvideoitem.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
        QtMultimediaMockBackend
)

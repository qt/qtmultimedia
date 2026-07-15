# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qgraphicsvideoitem Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qgraphicsvideoitem
    SOURCES
        tst_qgraphicsvideoitem.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
        Qt::MockMultimediaPlugin
)

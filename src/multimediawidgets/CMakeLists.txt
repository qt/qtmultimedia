# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## MultimediaWidgets Module:
#####################################################################

qt_internal_add_module(MultimediaWidgets
    SOURCES
        qtmultimediawidgetsglobal.h
        qvideowidget.cpp qvideowidget.h qvideowidget_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Multimedia
        Qt::Widgets
    PRIVATE_MODULE_INTERFACE
        Qt::MultimediaPrivate
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(MultimediaWidgets CONDITION QT_FEATURE_graphicsview
    SOURCES
        qgraphicsvideoitem.cpp qgraphicsvideoitem.h
)

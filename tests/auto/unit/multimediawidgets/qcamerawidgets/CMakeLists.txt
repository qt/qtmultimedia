# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcamerawidgets Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qcamerawidgets
    SOURCES
        tst_qcamerawidgets.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
        Qt::MockMultimediaPlugin
)

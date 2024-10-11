# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qvideoframe Test:
#####################################################################

qt_internal_add_test(tst_qvideoframe
    SOURCES
        tst_qvideoframe.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
)

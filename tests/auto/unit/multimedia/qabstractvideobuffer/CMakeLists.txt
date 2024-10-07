# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qabstractvideobuffer Test:
#####################################################################

qt_internal_add_test(tst_qabstractvideobuffer
    SOURCES
        tst_qabstractvideobuffer.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)

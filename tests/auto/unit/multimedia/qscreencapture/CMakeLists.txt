# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qscreencapture Test:
#####################################################################

qt_internal_add_test(tst_qscreencapture
    SOURCES
        tst_qscreencapture.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
)

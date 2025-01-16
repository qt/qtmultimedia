# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiodevice Test:
#####################################################################

qt_internal_add_test(tst_qaudiodevice
    SOURCES
        tst_qaudiodevice.cpp
    LIBRARIES
        Qt::MultimediaPrivate
)

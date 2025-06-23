# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiodevicebackend Test:
#####################################################################

qt_internal_add_test(tst_qaudiodevicebackend
    SOURCES
        tst_qaudiodevicebackend.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
)

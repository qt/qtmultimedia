# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmediatimerange Test:
#####################################################################

qt_internal_add_test(tst_qmediatimerange
    SOURCES
        tst_qmediatimerange.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
)

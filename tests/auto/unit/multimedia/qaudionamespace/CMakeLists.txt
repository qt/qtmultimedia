# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudionamespace Test:
#####################################################################

qt_internal_add_test(tst_qaudionamespace
    SOURCES
        tst_qaudionamespace.cpp
    LIBRARIES
        Qt::MultimediaPrivate
)

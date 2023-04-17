# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qaudiostatemachine
    SOURCES
        tst_qaudiostatemachine.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)


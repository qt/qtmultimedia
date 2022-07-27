# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qcamerabackend.pro.

#####################################################################
## tst_qcamerabackend Test:
#####################################################################

qt_internal_add_test(tst_qmediacapturesession
    SOURCES
        tst_qmediacapturesession.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgets
)

if(QT_FEATURE_gstreamer)
    set_tests_properties(tst_qmediacapturesession
        PROPERTIES ENVIRONMENT "G_DEBUG=fatal_criticals"
    )
endif()

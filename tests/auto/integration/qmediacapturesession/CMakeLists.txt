# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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

# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qdeclarativevideooutput_window.pro.

#####################################################################
## tst_qdeclarativevideooutput_window Test:
#####################################################################

qt_internal_add_test(tst_qquickvideooutput_window
    SOURCES
        tst_qquickvideooutput_window.cpp
    INCLUDE_DIRECTORIES
        ../../../../src/imports/multimedia
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt_internal_add_resource(tst_qquickvideooutput_window "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


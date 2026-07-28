# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdeclarativevideooutput Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qquickvideooutput
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qquickvideooutput.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        Qt::MultimediaTestLibPrivate
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt_internal_add_resource(tst_qquickvideooutput "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


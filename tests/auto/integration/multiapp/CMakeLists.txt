# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_multiapp
    SOURCES
        tst_multiapp.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
)

set(resources_resource_files
    "double-drop.wav"
)

qt_add_resources(tst_multiapp "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)

# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcamerabackend Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qcamerabackend
    NEEDS_MEDIA_BACKEND_PLUGINS
    MACOS_BUNDLE
    SOURCES
        tst_qcamerabackend.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Widgets
        Qt::MultimediaTestLibPrivate
)

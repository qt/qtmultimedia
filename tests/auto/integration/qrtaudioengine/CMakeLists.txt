# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_multimedia_add_test(tst_qrtaudioengine
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qrtaudioengine.cpp
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
)

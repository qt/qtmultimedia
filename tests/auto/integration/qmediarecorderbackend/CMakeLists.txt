# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_multimedia_add_test(tst_qmediarecorderbackend
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qmediarecorderbackend.cpp
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
)

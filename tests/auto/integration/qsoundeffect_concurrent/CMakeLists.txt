# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_multimedia_add_test(tst_qsoundeffect_concurrent
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qsoundeffect_concurrent.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    TESTDATA "double-drop.wav"
    BUILTIN_TESTDATA
)

# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiosource Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qaudiosource
    NEEDS_MEDIA_BACKEND_PLUGINS
    MACOS_BUNDLE
    SOURCES
        tst_qaudiosource.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
)

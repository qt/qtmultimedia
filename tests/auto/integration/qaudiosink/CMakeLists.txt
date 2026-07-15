# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiosink Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qaudiosink
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qaudiosink.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
)

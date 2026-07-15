# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcameradevice Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qcameradevice
    SOURCES
        tst_qcameradevice.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
)

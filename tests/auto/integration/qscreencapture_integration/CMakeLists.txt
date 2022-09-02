# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qscreencapture_integration Test:
#####################################################################

qt_internal_add_test(tst_qscreencapture_integration
    SOURCES
        tst_qscreencapture_integration.cpp
    LIBRARIES
        Qt::Multimedia
        Qt::Gui
        Qt::Widgets
)



# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwindowsasfpicture Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qwindowsasfpicture
    SOURCES
        tst_qwindowsasfpicture.cpp
        # NB: compiled directly into the test, as we don't have modular
        # static lib to test
        ../../../../../../../src/plugins/multimedia/windows/common/qwindowsasfpicture.cpp
    LIBRARIES
        Qt::Gui
)

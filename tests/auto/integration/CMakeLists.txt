# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from integration.pro.

# special case begin
add_subdirectory(qaudiodecoderbackend)
add_subdirectory(qaudiodevice)
add_subdirectory(qaudiosource)
add_subdirectory(qaudiosink)
add_subdirectory(qmediaplayerbackend)
add_subdirectory(qsoundeffect)
if(TARGET Qt::Widgets)
    add_subdirectory(qmediacapturesession)
    add_subdirectory(qcamerabackend)
    add_subdirectory(qscreencapturebackend)
    add_subdirectory(qwindowcapturebackend)
endif()
if(TARGET Qt::Quick)
    add_subdirectory(qquickvideooutput)
    add_subdirectory(qquickvideooutput_window)
endif()
# special case end

# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QLibCameraPlugin
    OUTPUT_NAME libcameraplugin
    PLUGIN_TYPE libcamera
    SOURCES
        qlibcamera_camera.cpp qlibcamera_camera_p.h
        qlibcamera_controls.cpp qlibcamera_controls_p.h
        qlibcamera_dmabufregistry.cpp qlibcamera_dmabufregistry_p.h
        qlibcamera_format_support.cpp qlibcamera_format_support_p.h
        qlibcamera_instance.cpp qlibcamera_instance_p.h
        qlibcamera_plugin.cpp
        qlibcamera_stream.cpp qlibcamera_stream_p.h
        qlibcamera_videodevices.cpp qlibcamera_videodevices_p.h
        qlibcameralogging_p.h
        libcamera.json
    DEFINES
        QT_NO_KEYWORDS
    LIBRARIES
        LibCamera::LibCamera
        Qt::MultimediaPrivate
)

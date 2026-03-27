# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_commandline_option(alsa TYPE boolean)
qt_commandline_option(gstreamer TYPE optionalString VALUES no yes)
qt_commandline_option(pulseaudio TYPE boolean)

qt_commandline_option(ffmpeg-dir TYPE path CMAKE_VARIABLE FFMPEG_DIR)
qt_commandline_option(ffmpeg-deploy TYPE boolean CMAKE_VARIABLE QT_DEPLOY_FFMPEG)

set(allowed_media_backends "ffmpeg" "gstreamer" "windows" "darwin" "android")
qt_commandline_option(default-media-backend TYPE optionalString CMAKE_VARIABLE QT_DEFAULT_MEDIA_BACKEND VALUES ${allowed_media_backends})

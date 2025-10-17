// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/QtCore>
#include <QtMultimedia/QtMultimedia>

#ifdef __OBJC__
// avfoundation and ffmpeg clash wrt AVMediaType
// #  import <AVFoundation/AVFoundation.h>
#  import <CoreVideo/CoreVideo.h>
#  import <dispatch/dispatch.h>
#  import <Metal/Metal.h>
#  ifdef Q_OS_MACOS
#    import <AppKit/AppKit.h>
#  endif
#  ifdef Q_OS_IOS
#    import <OpenGLES/EAGL.h>
#    import <UIKit/UIKit.h>
#  endif
#endif

#ifdef Q_OS_WINDOWS
#  include <qt_windows.h>
#  include <D3d11.h>
#  include <dxgi1_2.h>
#  include <mfapi.h>
#  include <mfidl.h>
#  include <mferror.h>
#  include <mfreadwrite.h>
#endif

// avfoundation and ffmpeg clash wrt AVMediaType
#ifndef __OBJC__
extern "C" {
#  include <libavformat/avformat.h>
#  include <libavcodec/avcodec.h>
#  include <libswresample/swresample.h>
#  include <libswscale/swscale.h>
#  include <libavutil/avutil.h>
}
#endif

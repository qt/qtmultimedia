// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/QtCore>
#include <QtMultimedia/QtMultimedia>

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreMedia/CoreMedia.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#ifdef Q_OS_MACOS
#import <AppKit/AppKit.h>
#import <VideoToolbox/VideoToolbox.h>
#import <ApplicationServices/ApplicationServices.h>
#import <AudioUnit/AudioUnit.h>
#endif

#ifdef Q_OS_IOS
#import <CoreGraphics/CoreGraphics.h>
#endif

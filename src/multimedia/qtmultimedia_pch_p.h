// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


#include <QtCore/qsystemdetection.h>

#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtConcurrent/QtConcurrent>

#ifdef Q_OS_MACOS
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>
#include <VideoToolbox/VideoToolbox.h>
#  ifdef __OBJC__
#  import <AVFoundation/AVFoundation.h>
#  endif
#endif

#ifdef Q_OS_WINDOWS
#  include <QtCore/qt_windows.h>

#  ifndef Q_CC_MINGW
#    include <audioclient.h>
#    include <ks.h>
#    include <ksmedia.h>
#    include <mfapi.h>
#    include <mferror.h>
#    include <mfidl.h>
#    include <mfobjects.h>
#    include <mfreadwrite.h>
#    include <mftransform.h>
#    include <mmdeviceapi.h>
#    include <mmreg.h>
#    include <propsys.h>
#    include <propvarutil.h>
#    include <wmcodecdsp.h>
#  endif // Q_CC_MINGW

#endif // Q_OS_WINDOWS

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSAUDIOUTILS_H
#define QWINDOWSAUDIOUTILS_H

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

#include <qaudioformat.h>
#include <QtCore/qt_windows.h>
#include <private/qwindowsiupointer_p.h>
#include <mfapi.h>
#include <mmsystem.h>
#include <mmreg.h>

struct IMFMediaType;

QT_BEGIN_NAMESPACE

namespace QWindowsAudioUtils
{
    bool formatToWaveFormatExtensible(const QAudioFormat &format, WAVEFORMATEXTENSIBLE &wfx);
    QAudioFormat waveFormatExToFormat(const WAVEFORMATEX &in);
    Q_MULTIMEDIA_EXPORT QAudioFormat mediaTypeToFormat(IMFMediaType *mediaType);
    QWindowsIUPointer<IMFMediaType> formatToMediaType(const QAudioFormat &format);
    QAudioFormat::ChannelConfig maskToChannelConfig(UINT32 mask, int count);
}

QT_END_NAMESPACE

#endif // QWINDOWSAUDIOUTILS_H

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
#include <QtCore/private/qcomptr_p.h>
#include <mmreg.h>

#include <optional>

struct IAudioClient;
struct IMFMediaType;

QT_BEGIN_NAMESPACE

class QWindowsMediaFoundation;

namespace QWindowsAudioUtils
{
    bool formatToWaveFormatExtensible(const QAudioFormat &format, WAVEFORMATEXTENSIBLE &wfx);
    std::optional<WAVEFORMATEXTENSIBLE> toWaveFormatExtensible(const QAudioFormat &format);

    QAudioFormat waveFormatExToFormat(const WAVEFORMATEX &in);
    Q_MULTIMEDIA_EXPORT QAudioFormat mediaTypeToFormat(IMFMediaType *mediaType);
    ComPtr<IMFMediaType> formatToMediaType(QWindowsMediaFoundation &, const QAudioFormat &format);
    QAudioFormat::ChannelConfig maskToChannelConfig(UINT32 mask, int count);
    std::optional<quint32> usedFrames(IAudioClient *client);
    std::optional<quint32> allocatedFrames(IAudioClient *client);
}

QT_END_NAMESPACE

#endif // QWINDOWSAUDIOUTILS_H

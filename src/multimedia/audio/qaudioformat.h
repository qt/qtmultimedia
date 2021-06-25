/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QAUDIOFORMAT_H
#define QAUDIOFORMAT_H

#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>

#include <QtMultimedia/qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

class QAudioFormatPrivate;

namespace QtPrivate {
template <typename... Args>
constexpr int channelConfig(Args... values) {
    return (0 | ... | (1u << values));
}
}

class QAudioFormat
{
public:
    enum SampleFormat : quint16 {
        Unknown,
        UInt8,
        Int16,
        Int32,
        Float,
        NSampleFormats
    };

    // This matches the speaker positions of a 22.2 audio layout. Stereo, Surround 5.1 and Surround 7.1 are subsets of these
    enum AudioChannelPosition {
        UnknownPosition,
        FrontLeft,
        FrontRight,
        FrontCenter,
        LFE,
        BackLeft,
        BackRight,
        FrontLeftOfCenter,
        FrontRightOfCenter,
        BackCenter,
        LFE2,
        SideLeft,
        SideRight,
        TopFrontLeft,
        TopFrontRight,
        TopFrontCenter,
        TopCenter,
        TopBackLeft,
        TopBackRight,
        TopSideLeft,
        TopSideRight,
        TopBackCenter,
        BottomFrontCenter,
        BottomFrontLeft,
        BottomFrontRight
    };

    enum ChannelConfig : quint32 {
        ChannelConfigUnknown = 0,
        ChannelConfigMono = QtPrivate::channelConfig(FrontCenter),
        ChannelConfigStereo = QtPrivate::channelConfig(FrontLeft, FrontRight),
        ChannelConfig2Dot1 = QtPrivate::channelConfig(FrontLeft, FrontRight, LFE),
        ChannelConfigSurround5Dot0 = QtPrivate::channelConfig(FrontLeft, FrontRight, FrontCenter, BackLeft, BackRight),
        ChannelConfigSurround5Dot1 = QtPrivate::channelConfig(FrontLeft, FrontRight, FrontCenter, LFE, BackLeft, BackRight),
        ChannelConfigSurround7Dot0 = QtPrivate::channelConfig(FrontLeft, FrontRight, FrontCenter, BackLeft, BackRight, SideLeft, SideRight),
        ChannelConfigSurround7Dot1 = QtPrivate::channelConfig(FrontLeft, FrontRight, FrontCenter, LFE, BackLeft, BackRight, SideLeft, SideRight),
    };

    template <typename... Args>
    static ChannelConfig channelConfig(Args... channels)
    {
        return ChannelConfig(QtPrivate::channelConfig(channels...));
    }

    constexpr bool isValid() const noexcept
    {
        return m_sampleRate > 0 && m_channelCount > 0 && m_sampleFormat != Unknown;
    }

    constexpr void setSampleRate(int sampleRate) noexcept { m_sampleRate = sampleRate; }
    constexpr int sampleRate() const noexcept { return m_sampleRate; }

    Q_MULTIMEDIA_EXPORT void setChannelConfig(ChannelConfig config) noexcept;
    constexpr ChannelConfig channelConfig() const noexcept { return m_channelConfig; }

    constexpr void setChannelCount(int channelCount) noexcept { m_channelConfig = ChannelConfigUnknown; m_channelCount = channelCount; }
    constexpr int channelCount() const noexcept { return m_channelCount; }

    Q_MULTIMEDIA_EXPORT int channelOffset(AudioChannelPosition channel) const noexcept;

    constexpr void setSampleFormat(SampleFormat f) noexcept { m_sampleFormat = f; }
    constexpr SampleFormat sampleFormat() const noexcept { return m_sampleFormat; }

    // Helper functions
    Q_MULTIMEDIA_EXPORT qint32 bytesForDuration(qint64 microseconds) const;
    Q_MULTIMEDIA_EXPORT qint64 durationForBytes(qint32 byteCount) const;

    Q_MULTIMEDIA_EXPORT qint32 bytesForFrames(qint32 frameCount) const;
    Q_MULTIMEDIA_EXPORT qint32 framesForBytes(qint32 byteCount) const;

    Q_MULTIMEDIA_EXPORT qint32 framesForDuration(qint64 microseconds) const;
    Q_MULTIMEDIA_EXPORT qint64 durationForFrames(qint32 frameCount) const;

    constexpr int bytesPerFrame() const { return bytesPerSample()*channelCount(); }
    constexpr int bytesPerSample() const noexcept
    {
        switch (m_sampleFormat) {
        case Unknown:
        case NSampleFormats: return 0;
        case UInt8: return 1;
        case Int16: return 2;
        case Int32:
        case Float: return 4;
        }
        return 0;
    }

    Q_MULTIMEDIA_EXPORT float normalizedSampleValue(const void *sample) const;

    friend bool operator==(const QAudioFormat &a, const QAudioFormat &b)
    {
        return a.m_sampleRate == b.m_sampleRate &&
               a.m_channelCount == b.m_channelCount &&
               a.m_sampleFormat == b.m_sampleFormat;
    }
    friend bool operator!=(const QAudioFormat &a, const QAudioFormat &b)
    {
        return !(a == b);
    }

private:
    SampleFormat m_sampleFormat = SampleFormat::Unknown;
    short m_channelCount = 0;
    ChannelConfig m_channelConfig = ChannelConfigUnknown;
    int m_sampleRate = 0;
    quint64 reserved = 0;
};

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, const QAudioFormat &);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QAudioFormat::SampleFormat);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QAudioFormat)

#endif  // QAUDIOFORMAT_H

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
#include <QtMultimedia/qmultimedia.h>

QT_BEGIN_NAMESPACE

class QAudioFormatPrivate;

class Q_MULTIMEDIA_EXPORT QAudioFormat
{
public:
    enum SampleFormat {
        Unknown,
        UInt8,
        Int16,
        Int32,
        Float,
        NSampleFormats
    };

    bool isValid() const;

    void setSampleRate(int sampleRate) { m_sampleRate = sampleRate; }
    int sampleRate() const { return m_sampleRate; }

    void setChannelCount(int channelCount) { m_channelCount = channelCount; }
    int channelCount() const { return m_channelCount; }

    void setSampleFormat(SampleFormat f) { m_sampleFormat = f; }
    SampleFormat sampleFormat() const { return m_sampleFormat; }

    // Helper functions
    qint32 bytesForDuration(qint64 duration) const;
    qint64 durationForBytes(qint32 byteCount) const;

    qint32 bytesForFrames(qint32 frameCount) const;
    qint32 framesForBytes(qint32 byteCount) const;

    qint32 framesForDuration(qint64 duration) const;
    qint64 durationForFrames(qint32 frameCount) const;

    int bytesPerFrame() const { return bytesPerSample()*channelCount(); }
    int bytesPerSample() const;

    float normalizedSampleValue(const void *sample) const;

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
    int m_sampleRate = 0;
    short m_channelCount = 0;
    SampleFormat m_sampleFormat = SampleFormat::Unknown;
};

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, const QAudioFormat &);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QAudioFormat::SampleFormat);
#endif

QT_END_NAMESPACE

#endif  // QAUDIOFORMAT_H

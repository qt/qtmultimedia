// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef IOSAUDIOUTILS_H
#define IOSAUDIOUTILS_H

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

#include <CoreAudio/CoreAudioTypes.h>

#include <QtMultimedia/QAudioFormat>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class CoreAudioUtils
{
public:
    static quint64 currentTime();
    static double frequency();
    static Q_MULTIMEDIA_EXPORT QAudioFormat toQAudioFormat(const AudioStreamBasicDescription& streamFormat);
    static AudioStreamBasicDescription toAudioStreamBasicDescription(QAudioFormat const& audioFormat);

    // ownership is transferred to caller, free with ::free()
    static Q_MULTIMEDIA_EXPORT std::unique_ptr<AudioChannelLayout> toAudioChannelLayout(const QAudioFormat &format, UInt32 *size);
    static QAudioFormat::ChannelConfig fromAudioChannelLayout(const AudioChannelLayout *layout);

private:
    static void initialize();
    static double sFrequency;
    static bool sIsInitialized;
};

class CoreAudioRingBuffer
{
public:
    typedef QPair<char*, int> Region;

    CoreAudioRingBuffer(int bufferSize);
    ~CoreAudioRingBuffer();

    Region acquireReadRegion(int size);
    void releaseReadRegion(Region const& region);
    Region acquireWriteRegion(int size);
    void releaseWriteRegion(Region const& region);

    int used() const;
    int free() const;
    int size() const;

    void reset();

private:
    int     m_bufferSize;
    int     m_readPos;
    int     m_writePos;
    char*   m_buffer;
    QAtomicInt  m_bufferUsed;
};

QT_END_NAMESPACE

#endif // IOSAUDIOUTILS_H

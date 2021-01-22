/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef IOSAUDIOUTILS_H
#define IOSAUDIOUTILS_H

#include <CoreAudio/CoreAudioTypes.h>

#include <QtMultimedia/QAudioFormat>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class CoreAudioUtils
{
public:
    static quint64 currentTime();
    static double frequency();
    static QAudioFormat toQAudioFormat(const AudioStreamBasicDescription& streamFormat);
    static AudioStreamBasicDescription toAudioStreamBasicDescription(QAudioFormat const& audioFormat);

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

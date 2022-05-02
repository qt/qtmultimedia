/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd and/or its subsidiary(-ies).
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
******************************************************************************/

#include "qcoreaudioutils_p.h"
#include <mach/mach_time.h>

QT_BEGIN_NAMESPACE

double CoreAudioUtils::sFrequency = 0.0;
bool CoreAudioUtils::sIsInitialized = false;

void CoreAudioUtils::initialize()
{
    struct mach_timebase_info timeBaseInfo;
    mach_timebase_info(&timeBaseInfo);
    sFrequency = static_cast<double>(timeBaseInfo.denom) / static_cast<double>(timeBaseInfo.numer);
    sFrequency *= 1000000000.0;

    sIsInitialized = true;
}


quint64 CoreAudioUtils::currentTime()
{
    return mach_absolute_time();
}

double CoreAudioUtils::frequency()
{
    if (!sIsInitialized)
        initialize();
    return sFrequency;
}

QAudioFormat CoreAudioUtils::toQAudioFormat(AudioStreamBasicDescription const& sf)
{
    QAudioFormat    audioFormat;
    // all Darwin HW is little endian, we ignore those formats
    if ((sf.mFormatFlags & kAudioFormatFlagIsBigEndian) != 0 && QSysInfo::ByteOrder != QSysInfo::LittleEndian)
        return audioFormat;

    // filter out the formats we're interested in
    QAudioFormat::SampleFormat format = QAudioFormat::Unknown;
    switch (sf.mBitsPerChannel) {
    case 8:
        if ((sf.mFormatFlags & kAudioFormatFlagIsSignedInteger) == 0)
            format = QAudioFormat::UInt8;
        break;
    case 16:
        if ((sf.mFormatFlags & kAudioFormatFlagIsSignedInteger) != 0)
            format = QAudioFormat::Int16;
        break;
    case 32:
        if ((sf.mFormatFlags & kAudioFormatFlagIsSignedInteger) != 0)
            format = QAudioFormat::Int32;
        else if ((sf.mFormatFlags & kAudioFormatFlagIsFloat) != 0)
            format = QAudioFormat::Float;
        break;
    default:
        break;
    }

    audioFormat.setSampleFormat(format);
    audioFormat.setSampleRate(sf.mSampleRate);
    audioFormat.setChannelCount(sf.mChannelsPerFrame);

    return audioFormat;
}

AudioStreamBasicDescription CoreAudioUtils::toAudioStreamBasicDescription(QAudioFormat const& audioFormat)
{
    AudioStreamBasicDescription sf;

    sf.mFormatFlags         = kAudioFormatFlagIsPacked;
    sf.mSampleRate          = audioFormat.sampleRate();
    sf.mFramesPerPacket     = 1;
    sf.mChannelsPerFrame    = audioFormat.channelCount();
    sf.mBitsPerChannel      = audioFormat.bytesPerSample() * 8;
    sf.mBytesPerFrame       = audioFormat.bytesPerFrame();
    sf.mBytesPerPacket      = sf.mFramesPerPacket * sf.mBytesPerFrame;
    sf.mFormatID            = kAudioFormatLinearPCM;

    switch (audioFormat.sampleFormat()) {
    case QAudioFormat::Int16:
    case QAudioFormat::Int32:
        sf.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
        break;
    case QAudioFormat::Float:
        sf.mFormatFlags |= kAudioFormatFlagIsFloat;
        break;
    case QAudioFormat::UInt8:
        /* default */
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats:
        break;
    }

    return sf;
}

// QAudioRingBuffer
CoreAudioRingBuffer::CoreAudioRingBuffer(int bufferSize):
    m_bufferSize(bufferSize)
{
    m_buffer = new char[m_bufferSize];
    reset();
}

CoreAudioRingBuffer::~CoreAudioRingBuffer()
{
    delete[] m_buffer;
}

CoreAudioRingBuffer::Region CoreAudioRingBuffer::acquireReadRegion(int size)
{
    const int used = m_bufferUsed.fetchAndAddAcquire(0);

    if (used > 0) {
        const int readSize = qMin(size, qMin(m_bufferSize - m_readPos, used));

        return readSize > 0 ? Region(m_buffer + m_readPos, readSize) : Region(0, 0);
    }

    return Region(0, 0);
}

void CoreAudioRingBuffer::releaseReadRegion(const CoreAudioRingBuffer::Region &region)
{
    m_readPos = (m_readPos + region.second) % m_bufferSize;

    m_bufferUsed.fetchAndAddRelease(-region.second);
}

CoreAudioRingBuffer::Region CoreAudioRingBuffer::acquireWriteRegion(int size)
{
    const int free = m_bufferSize - m_bufferUsed.fetchAndAddAcquire(0);

    Region output;

    if (free > 0) {
        const int writeSize = qMin(size, qMin(m_bufferSize - m_writePos, free));
        output =  writeSize > 0 ? Region(m_buffer + m_writePos, writeSize) : Region(0, 0);
    } else {
        output = Region(0, 0);
    }
#ifdef QT_DEBUG_COREAUDIO
    qDebug("acquireWriteRegion(%d) free: %d returning Region(%p, %d)", size, free, output.first, output.second);
#endif
    return output;
}
void CoreAudioRingBuffer::releaseWriteRegion(const CoreAudioRingBuffer::Region &region)
{
    m_writePos = (m_writePos + region.second) % m_bufferSize;

    m_bufferUsed.fetchAndAddRelease(region.second);
#ifdef QT_DEBUG_COREAUDIO
    qDebug("releaseWriteRegion(%p,%d): m_writePos:%d", region.first, region.second, m_writePos);
#endif
}

int CoreAudioRingBuffer::used() const
{
    return m_bufferUsed.loadRelaxed();
}

int CoreAudioRingBuffer::free() const
{
    return m_bufferSize - m_bufferUsed.loadRelaxed();
}

int CoreAudioRingBuffer::size() const
{
    return m_bufferSize;
}

void CoreAudioRingBuffer::reset()
{
    m_readPos = 0;
    m_writePos = 0;
    m_bufferUsed.storeRelaxed(0);
}

QT_END_NAMESPACE

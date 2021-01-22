/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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
    enum SampleType { Unknown, SignedInt, UnSignedInt, Float };
    enum Endian { BigEndian = QSysInfo::BigEndian, LittleEndian = QSysInfo::LittleEndian };

    QAudioFormat();
    QAudioFormat(const QAudioFormat &other);
    ~QAudioFormat();

    QAudioFormat& operator=(const QAudioFormat &other);
    bool operator==(const QAudioFormat &other) const;
    bool operator!=(const QAudioFormat &other) const;

    bool isValid() const;

    void setSampleRate(int sampleRate);
    int sampleRate() const;

    void setChannelCount(int channelCount);
    int channelCount() const;

    void setSampleSize(int sampleSize);
    int sampleSize() const;

    void setCodec(const QString &codec);
    QString codec() const;

    void setByteOrder(QAudioFormat::Endian byteOrder);
    QAudioFormat::Endian byteOrder() const;

    void setSampleType(QAudioFormat::SampleType sampleType);
    QAudioFormat::SampleType sampleType() const;

    // Helper functions
    qint32 bytesForDuration(qint64 duration) const;
    qint64 durationForBytes(qint32 byteCount) const;

    qint32 bytesForFrames(qint32 frameCount) const;
    qint32 framesForBytes(qint32 byteCount) const;

    qint32 framesForDuration(qint64 duration) const;
    qint64 durationForFrames(qint32 frameCount) const;

    int bytesPerFrame() const;

private:
    QSharedDataPointer<QAudioFormatPrivate> d;
};

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, const QAudioFormat &);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QAudioFormat::SampleType);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QAudioFormat::Endian);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QAudioFormat)
Q_DECLARE_METATYPE(QAudioFormat::SampleType)
Q_DECLARE_METATYPE(QAudioFormat::Endian)

#endif  // QAUDIOFORMAT_H

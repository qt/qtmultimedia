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


#ifndef QAUDIODEVICEINFO_H
#define QAUDIODEVICEINFO_H

#include <QtCore/qobject.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qlist.h>

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmultimedia.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudioformat.h>

QT_BEGIN_NAMESPACE


class QAudioDeviceFactory;

class QAudioDeviceInfoPrivate;
class Q_MULTIMEDIA_EXPORT QAudioDeviceInfo
{
    friend class QAudioDeviceFactory;

public:
    QAudioDeviceInfo();
    QAudioDeviceInfo(const QAudioDeviceInfo& other);
    ~QAudioDeviceInfo();

    QAudioDeviceInfo& operator=(const QAudioDeviceInfo& other);

    bool operator==(const QAudioDeviceInfo &other) const;
    bool operator!=(const QAudioDeviceInfo &other) const;

    bool isNull() const;

    QString deviceName() const;

    bool isFormatSupported(const QAudioFormat &format) const;
    QAudioFormat preferredFormat() const;
    QAudioFormat nearestFormat(const QAudioFormat &format) const;

    QStringList supportedCodecs() const;
    QList<int> supportedSampleRates() const;
    QList<int> supportedChannelCounts() const;
    QList<int> supportedSampleSizes() const;
    QList<QAudioFormat::Endian> supportedByteOrders() const;
    QList<QAudioFormat::SampleType> supportedSampleTypes() const;
    QString realm() const;

    static QAudioDeviceInfo defaultInputDevice();
    static QAudioDeviceInfo defaultOutputDevice();

    static QList<QAudioDeviceInfo> availableDevices(QAudio::Mode mode);

private:
    QAudioDeviceInfo(const QString &realm, const QByteArray &handle, QAudio::Mode mode);
    QByteArray handle() const;
    QAudio::Mode mode() const;

    QSharedDataPointer<QAudioDeviceInfoPrivate> d;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QAudioDeviceInfo)

#endif // QAUDIODEVICEINFO_H

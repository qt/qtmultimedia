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

#include <QtCore/qdebug.h>

#include "qaudiosystem_p.h"

#include "qaudiodevicefactory_p.h"

QT_BEGIN_NAMESPACE

class QNullDeviceInfo : public QAbstractAudioDeviceInfo
{
public:
    [[nodiscard]] QAudioFormat preferredFormat() const override { qWarning()<<"using null deviceinfo, none available"; return QAudioFormat(); }
    [[nodiscard]] bool isFormatSupported(const QAudioFormat& ) const override { return false; }
    [[nodiscard]] QAudioFormat nearestFormat(const QAudioFormat& ) const { return QAudioFormat(); }
    [[nodiscard]] QString deviceName() const override { return QString(); }
    QString description() const override { return deviceName(); };
    QStringList supportedCodecs() override { return QStringList(); }
    QList<int> supportedSampleRates() override  { return QList<int>(); }
    QList<int> supportedChannelCounts() override { return QList<int>(); }
    QList<int> supportedSampleSizes() override { return QList<int>(); }
    QList<QAudioFormat::Endian> supportedByteOrders() override { return QList<QAudioFormat::Endian>(); }
    QList<QAudioFormat::SampleType> supportedSampleTypes() override { return QList<QAudioFormat::SampleType>(); }
};

class QNullInputDevice : public QAbstractAudioInput
{
public:
    void start(QIODevice*) override { qWarning()<<"using null input device, none available";}
    QIODevice *start() override { qWarning()<<"using null input device, none available"; return nullptr; }
    void stop() override {}
    void reset() override {}
    void suspend() override {}
    void resume() override {}
    [[nodiscard]] int bytesReady() const override { return 0; }
    [[nodiscard]] int periodSize() const override { return 0; }
    void setBufferSize(int ) override {}
    [[nodiscard]] int bufferSize() const override  { return 0; }
    void setNotifyInterval(int ) override {}
    [[nodiscard]] int notifyInterval() const override { return 0; }
    [[nodiscard]] qint64 processedUSecs() const override { return 0; }
    [[nodiscard]] qint64 elapsedUSecs() const override { return 0; }
    [[nodiscard]] QAudio::Error error() const override { return QAudio::OpenError; }
    [[nodiscard]] QAudio::State state() const override { return QAudio::StoppedState; }
    void setFormat(const QAudioFormat&) override {}
    [[nodiscard]] QAudioFormat format() const override { return QAudioFormat(); }
    void setVolume(qreal) override {}
    [[nodiscard]] qreal volume() const override {return 1.0f;}
};

class QNullOutputDevice : public QAbstractAudioOutput
{
public:
    void start(QIODevice*) override {qWarning()<<"using null output device, none available";}
    QIODevice *start() override { qWarning()<<"using null output device, none available"; return nullptr; }
    void stop() override {}
    void reset() override {}
    void suspend() override {}
    void resume() override {}
    [[nodiscard]] int bytesFree() const override { return 0; }
    [[nodiscard]] int periodSize() const override { return 0; }
    void setBufferSize(int ) override {}
    [[nodiscard]] int bufferSize() const override  { return 0; }
    void setNotifyInterval(int ) override {}
    [[nodiscard]] int notifyInterval() const override { return 0; }
    [[nodiscard]] qint64 processedUSecs() const override { return 0; }
    [[nodiscard]] qint64 elapsedUSecs() const override { return 0; }
    [[nodiscard]] QAudio::Error error() const override { return QAudio::OpenError; }
    [[nodiscard]] QAudio::State state() const override { return QAudio::StoppedState; }
    void setFormat(const QAudioFormat&) override {}
    [[nodiscard]] QAudioFormat format() const override { return QAudioFormat(); }
};

QList<QAudioDeviceInfo> QAudioDeviceFactory::availableDevices(QAudio::Mode mode)
{
    QList<QAudioDeviceInfo> devices;
    auto *iface = QAudioSystemInterface::instance();
    if (iface) {
        const auto availableDevices = iface->availableDevices(mode);
        for (const QByteArray& handle : availableDevices)
            devices << QAudioDeviceInfo(handle, mode);
    }

    return devices;
}

QAudioDeviceInfo QAudioDeviceFactory::defaultDevice(QAudio::Mode mode)
{
    auto *iface = QAudioSystemInterface::instance();
    if (iface) {
        // Ask for the default device.
        const QByteArray &device = iface->defaultDevice(mode);
        if (!device.isEmpty())
            return QAudioDeviceInfo(device, mode);

        // If there were no default devices then just pick the first device that's available.
        const auto &devices = iface->availableDevices(mode);
        if (!devices.isEmpty())
            return QAudioDeviceInfo(devices.first(), mode);
    }

    return QAudioDeviceInfo();
}

QAbstractAudioDeviceInfo* QAudioDeviceFactory::audioDeviceInfo(const QByteArray &handle, QAudio::Mode mode)
{
    QAbstractAudioDeviceInfo *rc = nullptr;

    auto *iface = QAudioSystemInterface::instance();
    if (iface)
        rc = iface->createDeviceInfo(handle, mode);

    return rc == nullptr ? new QNullDeviceInfo() : rc;
}

QAbstractAudioInput* QAudioDeviceFactory::createInputDevice(const QAudioFormat &format, const QAudioDeviceInfo &deviceInfo)
{
    QAudioDeviceInfo info = deviceInfo;
    if (info.isNull())
        info = defaultDevice(QAudio::AudioInput);

    auto *iface = QAudioSystemInterface::instance();
    if (iface) {
        QAbstractAudioInput* p = iface->createInput(info.handle());
        if (p) p->setFormat(format);
        return p;
    }

    return new QNullInputDevice();
}

QAbstractAudioOutput* QAudioDeviceFactory::createOutputDevice(const QAudioFormat &format, const QAudioDeviceInfo &deviceInfo)
{
    QAudioDeviceInfo info = deviceInfo;
    if (info.isNull())
        info = defaultDevice(QAudio::AudioOutput);

    auto *iface = QAudioSystemInterface::instance();
    if (iface) {
        QAbstractAudioOutput* p = iface->createOutput(info.handle());
        if (p) p->setFormat(format);
        return p;
    }

    return new QNullOutputDevice();
}

QT_END_NAMESPACE


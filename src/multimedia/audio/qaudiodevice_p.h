// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QAUDIODEVICEINFO_P_H
#define QAUDIODEVICEINFO_P_H

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

#include <QtMultimedia/qaudiodevice.h>
#include <QtCore/private/qglobal_p.h>
#include <QtCore/private/qexpected_p.h>

#include <future>
#include <chrono>

QT_BEGIN_NAMESPACE

enum class QAudioDeviceFormatError : uint8_t {
    InvalidFuture,
    Timeout,
};

template<typename T>
using QAudioDeviceExpected = q23::expected<T, QAudioDeviceFormatError>;

// Implementations should not include volatile members, such as values  that can change between
// connection sessions. For example, CoreAudio AudioDeviceId on macOS.
class Q_MULTIMEDIA_EXPORT QAudioDevicePrivate : public QSharedData
{
public:
    struct AudioDeviceFormat
    {
        QAudioFormat preferredFormat;
        int minimumSampleRate = 0;
        int maximumSampleRate = 0;
        int minimumChannelCount = 0;
        int maximumChannelCount = 0;
        QList<QAudioFormat::SampleFormat> supportedSampleFormats;
        QAudioFormat::ChannelConfig channelConfiguration = QAudioFormat::ChannelConfigUnknown;

        friend bool operator==(const AudioDeviceFormat &lhs, const AudioDeviceFormat &rhs)
        {
            return lhs.preferredFormat == rhs.preferredFormat
                    && lhs.minimumSampleRate == rhs.minimumSampleRate
                    && lhs.maximumSampleRate == rhs.maximumSampleRate
                    && lhs.minimumChannelCount == rhs.minimumChannelCount
                    && lhs.maximumChannelCount == rhs.maximumChannelCount
                    && lhs.supportedSampleFormats == rhs.supportedSampleFormats
                    && lhs.channelConfiguration == rhs.channelConfiguration;
        }
    };

    QAudioDevicePrivate(QByteArray i, QAudioDevice::Mode m, QString description, bool isDefault,
                        std::future<AudioDeviceFormat> format);

    QAudioDevicePrivate(const QByteArray &i, QAudioDevice::Mode m, QString description,
                        bool isDefault, AudioDeviceFormat format);

    virtual ~QAudioDevicePrivate();
    const QByteArray id;
    const QAudioDevice::Mode mode = QAudioDevice::Output;
    const QString description;
    bool isDefault = false;

    QAudioDeviceExpected<AudioDeviceFormat> format() const;

    QAudioDeviceExpected<QAudioFormat> preferredFormat() const;
    QAudioDeviceExpected<int> minimumSampleRate() const;
    QAudioDeviceExpected<int> maximumSampleRate() const;
    QAudioDeviceExpected<int> minimumChannelCount() const;
    QAudioDeviceExpected<int> maximumChannelCount() const;
    QAudioDeviceExpected<QList<QAudioFormat::SampleFormat>> supportedSampleFormats() const;
    QAudioDeviceExpected<QAudioFormat::ChannelConfig> channelConfiguration() const;

    QAudioDeviceExpected<bool> isFormatSupported(const QAudioFormat &format) const;

    static QAudioDevice createQAudioDevice(std::unique_ptr<QAudioDevicePrivate> devicePrivate);

    static const QAudioDevicePrivate *handle(const QAudioDevice &device);

    template <typename Derived>
    static const Derived *handle(const QAudioDevice &device)
    {
        // Note: RTTI is required for dispatching in the gstreamer backend
        return dynamic_cast<const Derived *>(handle(device));
    }

    static constexpr std::chrono::seconds formatProbeTimeout{4};

protected:
    QAudioDevicePrivate(const QAudioDevicePrivate &) = default;

private:
    template <typename F>
    QAudioDeviceExpected<std::invoke_result_t<F, const AudioDeviceFormat &>>
    doWithDeviceFormat(F &&f) const;

    std::shared_future<AudioDeviceFormat> m_deviceFormat;
};

inline const QList<QAudioFormat::SampleFormat> &qAllSupportedSampleFormats()
{
    static const auto singleton = QList<QAudioFormat::SampleFormat>{
        QAudioFormat::UInt8,
        QAudioFormat::Int16,
        QAudioFormat::Int32,
        QAudioFormat::Float,
    };
    return singleton;
}

struct QAudioDevicePrivateAllMembersEqual
{
    bool operator()(const QAudioDevicePrivate &lhs, const QAudioDevicePrivate &rhs)
    {
        if (lhs.id != rhs.id || lhs.mode != rhs.mode || lhs.isDefault != rhs.isDefault || lhs.description != rhs.description)
            return false;

        return lhs.format() == rhs.format();
    }
};

QT_END_NAMESPACE

#endif // QAUDIODEVICEINFO_H

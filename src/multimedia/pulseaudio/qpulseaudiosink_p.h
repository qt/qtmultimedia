// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPULSEAUDIOSINK_P_H
#define QPULSEAUDIOSINK_P_H

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

#include <QtCore/qtclasshelpermacros.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qaudiosystem_p.h>

QT_BEGIN_NAMESPACE

namespace QPulseAudioInternal {
struct QPulseAudioSinkStream;
} // namespace QPulseAudioInternal

class QPulseAudioSink final : public QPlatformAudioSink
{
    friend class PulseOutputPrivate;

public:
    QPulseAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent);
    Q_DISABLE_COPY_MOVE(QPulseAudioSink)
    ~QPulseAudioSink() override;

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesFree() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    qint64 processedUSecs() const override;

    void setVolume(float volume) override;

private:
    using QPulseAudioSinkStream = QPulseAudioInternal::QPulseAudioSinkStream;
    friend QPulseAudioSinkStream;
    friend QtMultimediaPrivate::QPlatformAudioSinkStream;
    std::shared_ptr<QPulseAudioSinkStream> m_stream;

    std::optional<qsizetype> m_bufferSize;
    std::optional<qsizetype> m_hardwareBufferFrames;
    AudioEndpointRole m_role = AudioEndpointRole::Other;

    template <typename Functor>
    void startHelper(Functor &&starter);
};

QT_END_NAMESPACE

#endif // QPULSEAUDIOSINK_P_H

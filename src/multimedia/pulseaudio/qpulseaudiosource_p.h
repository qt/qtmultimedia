// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QPULSEAUDIOSOURCE_P_H
#define QPULSEAUDIOSOURCE_P_H

#include <QtCore/qtclasshelpermacros.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qaudiosystem_p.h>

QT_BEGIN_NAMESPACE

namespace QPulseAudioInternal {
struct QPulseAudioSourceStream;
} // namespace QPulseAudioInternal

class QPulseAudioSource final : public QPlatformAudioSource
{
public:
    QPulseAudioSource(QAudioDevice device, const QAudioFormat &, QObject *parent);
    Q_DISABLE_COPY_MOVE(QPulseAudioSource)
    ~QPulseAudioSource() override;

    qint64 read(char *data, qint64 len);

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesReady() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    qint64 processedUSecs() const override;

private:
    using QPulseAudioSourceStream = QPulseAudioInternal::QPulseAudioSourceStream;
    friend QtMultimediaPrivate::QPlatformAudioSourceStream;
    friend QPulseAudioSourceStream;
    std::shared_ptr<QPulseAudioSourceStream> m_stream;
    std::shared_ptr<QPulseAudioSourceStream> m_retiredStream;

    std::optional<qsizetype> m_bufferSize;
    std::optional<qsizetype> m_hardwareBufferFrames;

    template <typename Functor>
    void startHelper(Functor &&starter);
};

QT_END_NAMESPACE

#endif // QPULSEAUDIOSOURCE_P_H

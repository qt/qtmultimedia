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

#ifndef QWINDOWSAUDIOSOURCE_H
#define QWINDOWSAUDIOSOURCE_H

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qaudiosystem_p.h>

QT_BEGIN_NAMESPACE

struct QWASAPIAudioSourceStream;

class QWindowsAudioSource final : public QPlatformAudioSource
{
    Q_OBJECT

public:
    QWindowsAudioSource(QAudioDevice, const QAudioFormat &fmt, QObject *parent);
    ~QWindowsAudioSource();

    QAudioFormat format() const override;
    QIODevice* start() override;
    void start(QIODevice* device) override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesReady() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    qint64 processedUSecs() const override;
    void setVolume(qreal volume) override;
    qreal volume() const override;

private:
    friend struct QWASAPIAudioSourceStream;

    const QAudioFormat m_format;
    std::optional<qsizetype> m_bufferSize;
    std::optional<qsizetype> m_hardwareBufferSize;

    qreal m_volume = 1.0;
    std::shared_ptr<QWASAPIAudioSourceStream> m_stream;
};

QT_END_NAMESPACE

#endif

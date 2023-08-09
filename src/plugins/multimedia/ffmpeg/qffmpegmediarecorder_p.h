// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGMEDIARECODER_H
#define QFFMPEGMEDIARECODER_H

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

#include <private/qplatformmediarecorder_p.h>

QT_BEGIN_NAMESPACE

class QAudioSource;
class QAudioSourceIO;
class QAudioBuffer;
class QMediaMetaData;
class QFFmpegMediaCaptureSession;

namespace QFFmpeg {
class Encoder;
}

class QFFmpegMediaRecorder : public QObject, public QPlatformMediaRecorder
{
    Q_OBJECT
public:
    QFFmpegMediaRecorder(QMediaRecorder *parent);
    virtual ~QFFmpegMediaRecorder();

    bool isLocationWritable(const QUrl &sink) const override;

    void record(QMediaEncoderSettings &settings) override;
    void pause() override;
    void resume() override;
    void stop() override;

    void setMetaData(const QMediaMetaData &) override;
    QMediaMetaData metaData() const override;

    void setCaptureSession(QFFmpegMediaCaptureSession *session);

private Q_SLOTS:
    void newDuration(qint64 d) { durationChanged(d); }
    void finalizationDone();
    void handleSessionError(QMediaRecorder::Error code, const QString &description);

private:
    using Encoder = QFFmpeg::Encoder;
    struct EncoderDeleter
    {
        void operator()(Encoder *) const;
    };

    QFFmpegMediaCaptureSession *m_session = nullptr;
    QMediaMetaData m_metaData;

    std::unique_ptr<Encoder, EncoderDeleter> m_encoder;
};

QT_END_NAMESPACE

#endif // QFFMPEGMEDIARECODER_H

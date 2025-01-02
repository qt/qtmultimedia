// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERMEDIARECORDER_H
#define QGSTREAMERMEDIARECORDER_H

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

#include <mediacapture/qgstreamermediacapturesession_p.h>
#include <common/qgstreamermetadata_p.h>

#include <QtMultimedia/private/qplatformmediarecorder_p.h>
#include <QtCore/qurl.h>
#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

class QMediaMetaData;
class QGstreamerMessage;

class QGstreamerMediaRecorder : public QPlatformMediaRecorder
{
public:
    explicit QGstreamerMediaRecorder(QMediaRecorder *parent);
    ~QGstreamerMediaRecorder() override;

    bool isLocationWritable(const QUrl &sink) const override;

    qint64 duration() const override;

    void record(QMediaEncoderSettings &settings) override;
    void pause() override;
    void resume() override;
    void stop() override;

    void setMetaData(const QMediaMetaData &) override;
    QMediaMetaData metaData() const override;

    void setCaptureSession(QPlatformMediaCaptureSession *session);

    void processBusMessage(const QGstreamerMessage &message);

private:
    struct PauseControl {
        explicit PauseControl(QPlatformMediaRecorder &encoder) : encoder(encoder) { }

        GstPadProbeReturn processBuffer(QGstPad pad, GstPadProbeInfo *info);
        void installOn(QGstPad pad);
        void reset();

        QPlatformMediaRecorder &encoder;
        GstClockTime pauseOffsetPts = 0;
        std::optional<GstClockTime> pauseStartPts;
        std::optional<GstClockTime> firstBufferPts;
        qint64 duration = 0;
    };

    PauseControl audioPauseControl;
    PauseControl videoPauseControl;

    void handleSessionError(QMediaRecorder::Error code, const QString &description);
    void finalize();

    QGstreamerMediaCaptureSession *m_session = nullptr;
    QMediaMetaData m_metaData;
    QTimer signalDurationChangedTimer;

    bool m_finalizing = false;
};

QT_END_NAMESPACE

#endif // QGSTREAMERMEDIARECORDER_H

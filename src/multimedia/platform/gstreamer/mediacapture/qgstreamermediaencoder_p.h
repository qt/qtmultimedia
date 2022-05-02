/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/


#ifndef QGSTREAMERENCODERCONTROL_H
#define QGSTREAMERENCODERCONTROL_H

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
#include "qgstreamermediacapture_p.h"
#include "private/qgstreamermetadata_p.h"

#include <QtCore/qurl.h>
#include <QtCore/qdir.h>
#include <qelapsedtimer.h>
#include <qtimer.h>

QT_BEGIN_NAMESPACE

class QMediaMetaData;
class QGstreamerMessage;

class QGstreamerMediaEncoder : public QPlatformMediaRecorder, QGstreamerBusMessageFilter
{
public:
    QGstreamerMediaEncoder(QMediaRecorder *parent);
    virtual ~QGstreamerMediaEncoder();

    bool isLocationWritable(const QUrl &sink) const override;

    qint64 duration() const override;

    void record(QMediaEncoderSettings &settings) override;
    void pause() override;
    void resume() override;
    void stop() override;

    void setMetaData(const QMediaMetaData &) override;
    QMediaMetaData metaData() const override;

    void setCaptureSession(QPlatformMediaCaptureSession *session);

    QGstElement getEncoder() { return gstEncoder; }
private:
    bool processBusMessage(const QGstreamerMessage& message) override;

private:
    struct PauseControl {
        PauseControl(QPlatformMediaRecorder &encoder) : encoder(encoder) {}

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

    QGstreamerMediaCapture *m_session = nullptr;
    QGstreamerMetaData m_metaData;
    QTimer signalDurationChangedTimer;

    QGstPipeline gstPipeline;
    QGstBin gstEncoder;
    QGstElement gstFileSink;

    bool m_finalizing = false;
};

QT_END_NAMESPACE

#endif // QGSTREAMERENCODERCONTROL_H

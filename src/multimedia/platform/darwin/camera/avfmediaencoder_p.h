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

#ifndef AVFMEDIAENCODER_H
#define AVFMEDIAENCODER_H

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

#include "avfmediaassetwriter_p.h"
#include "avfcamerautility_p.h"
#include "qaudiodevice.h"

#include <private/qplatformmediarecorder_p.h>
#include <private/qplatformmediacapture_p.h>
#include <QtMultimedia/qmediametadata.h>

#include <QtCore/qglobal.h>
#include <QtCore/qurl.h>

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFCameraService;
class QString;
class QUrl;

class AVFMediaEncoder : public QObject, public QPlatformMediaRecorder
{
    Q_OBJECT
public:
    AVFMediaEncoder(QMediaRecorder *parent);
    ~AVFMediaEncoder() override;

    bool isLocationWritable(const QUrl &location) const override;

    QMediaRecorder::RecorderState state() const override;

    qint64 duration() const override;

    void record(QMediaEncoderSettings &settings) override;
    void pause() override;
    void resume() override;
    void stop() override;

    void setMetaData(const QMediaMetaData &) override;
    QMediaMetaData metaData() const override;

    AVFCameraService *cameraService() const { return m_service; }

    void setCaptureSession(QPlatformMediaCaptureSession *session);

    void updateDuration(qint64 duration);

    void toggleRecord(bool enable);

private:
    void applySettings(QMediaEncoderSettings &settings);
    void unapplySettings();

    Q_INVOKABLE void assetWriterStarted();
    Q_INVOKABLE void assetWriterFinished();
    Q_INVOKABLE void assetWriterError(QString error);

private Q_SLOTS:
    void onCameraChanged();
    void cameraActiveChanged(bool);

private:
    void stopWriter();

    AVFCameraService *m_service = nullptr;
    AVFScopedPointer<QT_MANGLE_NAMESPACE(AVFMediaAssetWriter)> m_writer;

    QMediaRecorder::RecorderState m_state;

    QMediaMetaData m_metaData;

    qint64 m_duration;

    NSDictionary *m_audioSettings;
    NSDictionary *m_videoSettings;
};

QT_END_NAMESPACE

#endif // AVFMEDIAENCODER_H

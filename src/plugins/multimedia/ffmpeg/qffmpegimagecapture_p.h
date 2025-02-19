// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QFFMPEGIMAGECAPTURE_H
#define QFFMPEGIMAGECAPTURE_H

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

#include <QtMultimedia/private/qplatformimagecapture_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegmediacapturesession_p.h>

#include <QtCore/qpointer.h>
#include <qqueue.h>

QT_BEGIN_NAMESPACE

class QFFmpegImageCapture : public QPlatformImageCapture
{
    Q_OBJECT
public:
    QFFmpegImageCapture(QImageCapture *parent);
    ~QFFmpegImageCapture() override;

    bool isReadyForCapture() const override;
    int capture(const QString &fileName) override;
    int captureToBuffer() override;

    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

    void setCaptureSession(QPlatformMediaCaptureSession *session);

protected:
    virtual int doCapture(const QString &fileName);
    virtual void setupVideoSourceConnections();
    QPlatformVideoSource *videoSource() const;
    void updateReadyForCapture();

protected Q_SLOTS:
    void newVideoFrame(const QVideoFrame &frame);
    void onVideoSourceChanged();

private:
    QFFmpegMediaCaptureSession *m_session = nullptr;
    QPointer<QPlatformVideoSource> m_videoSource;
    int m_lastId = 0;
    QImageEncoderSettings m_settings;

    struct PendingImage {
        int id;
        QString filename;
        QMediaMetaData metaData;
    };

    QQueue<PendingImage> m_pendingImages;
    bool m_isReadyForCapture = false;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURECORNTROL_H

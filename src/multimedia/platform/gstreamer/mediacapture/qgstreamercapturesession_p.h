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

#ifndef QGSTREAMERCAPTURESESSION_H
#define QGSTREAMERCAPTURESESSION_H

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

#include <qmediarecordercontrol.h>
#include <qmediarecorder.h>
#include <qaudiodeviceinfo.h>

#include <QtCore/qmutex.h>
#include <QtCore/qurl.h>

#include <gst/gst.h>
#include <gst/video/video.h>

#include <private/qgstreamerbushelper_p.h>
#include <private/qgstreamerbufferprobe_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerMessage;
class QGstreamerBusHelper;
class QGstreamerImageCaptureControl;
class QGstreamerRecorderControl;
class QGstreamerVideoRendererInterface;
class QCameraInfo;
class QGstreamerVideoInput;

class QGstreamerElementFactory
{
public:
    virtual GstElement *buildElement() = 0;
    virtual void prepareWinId() {}
};

class QGstreamerCaptureSession
        : public QObject
        , public QGstreamerBusMessageFilter
        , private QGstreamerBufferProbe
{
    Q_OBJECT
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_ENUMS(State)
    Q_ENUMS(CaptureMode)
    Q_INTERFACES(QGstreamerBusMessageFilter)
public:
    enum CaptureMode { Audio = 1,
                       Video = 2,
                       Image = 4,
                       AudioAndVideo = Audio | Video,
                       AudioAndVideoAndImage = Audio | Video | Image
                     };
    enum State { StoppedState, PreviewState, PausedState, RecordingState };

    QGstreamerCaptureSession(CaptureMode captureMode, QObject *parent);
    ~QGstreamerCaptureSession();

    QGstreamerBusHelper *bus() { return m_busHelper; }

    CaptureMode captureMode() const { return m_captureMode; }
    void setCaptureMode(CaptureMode);

    QUrl outputLocation() const;
    bool setOutputLocation(const QUrl& sink);

    QGstreamerImageCaptureControl *imageCaptureControl() const { return m_imageCaptureControl; }

    QGstreamerRecorderControl *recorderControl() const { return m_recorderControl; }

    QGstreamerElementFactory *audioPreview() const { return m_audioPreviewFactory; }
    void setAudioPreview(QGstreamerElementFactory *audioPreview);

    QGstreamerVideoInput *videoInput() const { return m_videoInputFactory; }
    void setVideoInput(QGstreamerVideoInput *videoInput);

    QObject *videoPreview() const { return m_viewfinder; }
    void setVideoPreview(QObject *viewfinder);

    void captureImage(int requestId, const QString &fileName);

    State state() const;
    State pendingState() const;

    qint64 duration() const;
    bool isMuted() const { return m_muted; }
    qreal volume() const { return m_volume; }

    bool isReady() const;

    bool processBusMessage(const QGstreamerMessage &message) override;

    QAudioDeviceInfo audioCaptureDevice() const { return m_audioDevice; }

signals:
    void stateChanged(QGstreamerCaptureSession::State state);
    void durationChanged(qint64 duration);
    void error(int error, const QString &errorString);
    void imageExposed(int requestId);
    void imageCaptured(int requestId, const QImage &img);
    void imageSaved(int requestId, const QString &path);
    void mutedChanged(bool);
    void volumeChanged(qreal);
    void readyChanged(bool);
    void viewfinderChanged();

public slots:
    void setState(QGstreamerCaptureSession::State);
    void setAudioCaptureDevice(const QAudioDeviceInfo &audioDevice);

    void dumpGraph(const QString &fileName);

    void setMetaData(const QMap<QByteArray, QVariant>&);
    void setMuted(bool);
    void setVolume(qreal volume);

private:
    void probeCaps(GstCaps *caps) override;
    bool probeBuffer(GstBuffer *buffer) override;

    enum PipelineMode { EmptyPipeline, PreviewPipeline, RecordingPipeline, PreviewAndRecordingPipeline };

    GstElement *buildEncodeBin();
    GstElement *buildFileSink();
    GstElement *buildAudioSrc();
    GstElement *buildAudioPreview();
    GstElement *buildVideoSrc();
    GstElement *buildVideoPreview();
    GstElement *buildImageCapture();

    bool rebuildGraph(QGstreamerCaptureSession::PipelineMode newMode);

    QAudioDeviceInfo m_audioDevice;
    QUrl m_sink;
    State m_state;
    State m_pendingState;
    bool m_waitingForEos;
    PipelineMode m_pipelineMode;
    QGstreamerCaptureSession::CaptureMode m_captureMode;
    QMap<QByteArray, QVariant> m_metaData;

    QGstreamerElementFactory *m_audioPreviewFactory;
    QGstreamerVideoInput *m_videoInputFactory;
    QObject *m_viewfinder;
    QGstreamerVideoRendererInterface *m_viewfinderInterface;

    QGstreamerImageCaptureControl *m_imageCaptureControl;
    QGstreamerRecorderControl *m_recorderControl;

    QGstreamerBusHelper *m_busHelper;
    GstBus* m_bus;
    GstElement *m_pipeline;

    GstElement *m_audioSrc;
    GstElement *m_audioTee;
    GstElement *m_audioPreviewQueue;
    GstElement *m_audioPreview;
    GstElement *m_audioVolume;
    gboolean m_muted;
    double m_volume;

    GstElement *m_videoSrc;
    GstElement *m_videoTee;
    GstElement *m_videoPreviewQueue;
    GstElement *m_videoPreview;

    GstElement *m_imageCaptureBin;

    GstElement *m_encodeBin;
    GstElement *m_fileSink = nullptr;

    GstVideoInfo m_previewInfo;

public:
    bool m_passImage;
    bool m_passPrerollImage;
    QString m_imageFileName;
    int m_imageRequestId;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURESESSION_H

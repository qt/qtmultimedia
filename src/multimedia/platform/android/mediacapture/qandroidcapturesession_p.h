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

#ifndef QANDROIDCAPTURESESSION_H
#define QANDROIDCAPTURESESSION_H

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

#include <qobject.h>
#include <qmediarecorder.h>
#include <qurl.h>
#include <qelapsedtimer.h>
#include <qtimer.h>
#include "androidmediarecorder_p.h"
#include "qandroidmediaencoder_p.h"

QT_BEGIN_NAMESPACE

class QAudioInput;
class QAndroidCameraSession;

class QAndroidCaptureSession : public QObject
{
    Q_OBJECT
public:
    explicit QAndroidCaptureSession();
    ~QAndroidCaptureSession();

    QList<QSize> supportedResolutions() const { return m_supportedResolutions; }
    QList<qreal> supportedFrameRates() const { return m_supportedFramerates; }

    void setCameraSession(QAndroidCameraSession *cameraSession = 0);
    void setAudioInput(QPlatformAudioInput *input);
    void setAudioOutput(QPlatformAudioOutput *output);

    QMediaRecorder::RecorderState state() const;

    void start(QMediaEncoderSettings &settings, const QUrl &outputLocation);
    void stop(bool error = false);

    qint64 duration() const;

    QMediaEncoderSettings encoderSettings() { return m_encoderSettings; }

    void setMediaEncoder(QAndroidMediaEncoder *encoder) { m_mediaEncoder = encoder; }

    void stateChanged(QMediaRecorder::RecorderState state) {
        if (m_mediaEncoder)
            m_mediaEncoder->stateChanged(state);
    }
    void durationChanged(qint64 position)
    {
        if (m_mediaEncoder)
            m_mediaEncoder->durationChanged(position);
    }
    void actualLocationChanged(const QUrl &location)
    {
        if (m_mediaEncoder)
            m_mediaEncoder->actualLocationChanged(location);
    }
    void error(int error, const QString &errorString)
    {
        if (m_mediaEncoder)
            m_mediaEncoder->error(QMediaRecorder::Error(error), errorString);
    }

private Q_SLOTS:
    void updateDuration();
    void onCameraOpened();

    void onError(int what, int extra);
    void onInfo(int what, int extra);

private:
    void applySettings(QMediaEncoderSettings &settings);

    struct CaptureProfile {
        AndroidMediaRecorder::OutputFormat outputFormat;
        QString outputFileExtension;

        AndroidMediaRecorder::AudioEncoder audioEncoder;
        int audioBitRate;
        int audioChannels;
        int audioSampleRate;

        AndroidMediaRecorder::VideoEncoder videoEncoder;
        int videoBitRate;
        int videoFrameRate;
        QSize videoResolution;

        bool isNull;

        CaptureProfile()
            : outputFormat(AndroidMediaRecorder::MPEG_4)
            , outputFileExtension(QLatin1String("mp4"))
            , audioEncoder(AndroidMediaRecorder::DefaultAudioEncoder)
            , audioBitRate(128000)
            , audioChannels(2)
            , audioSampleRate(44100)
            , videoEncoder(AndroidMediaRecorder::DefaultVideoEncoder)
            , videoBitRate(1)
            , videoFrameRate(-1)
            , videoResolution(1280, 720)
            , isNull(true)
        { }
    };

    CaptureProfile getProfile(int id);

    void restartViewfinder();

    QAndroidMediaEncoder *m_mediaEncoder = nullptr;
    std::shared_ptr<AndroidMediaRecorder> m_mediaRecorder;
    QAndroidCameraSession *m_cameraSession;

    QPlatformAudioInput *m_audioInput = nullptr;
    QPlatformAudioOutput *m_audioOutput = nullptr;

    QElapsedTimer m_elapsedTime;
    QTimer m_notifyTimer;
    qint64 m_duration;

    QMediaRecorder::RecorderState m_state;
    QUrl m_usedOutputLocation;
    bool m_outputLocationIsStandard = false;

    CaptureProfile m_defaultSettings;

    QMediaEncoderSettings m_encoderSettings;
    AndroidMediaRecorder::OutputFormat m_outputFormat;
    AndroidMediaRecorder::AudioEncoder m_audioEncoder;
    AndroidMediaRecorder::VideoEncoder m_videoEncoder;

    QList<QSize> m_supportedResolutions;
    QList<qreal> m_supportedFramerates;

    QMetaObject::Connection m_connOpenCamera;
    QMetaObject::Connection m_connActiveChangedCamera;

    void setKeepAlive(bool keepAlive);

};

QT_END_NAMESPACE

#endif // QANDROIDCAPTURESESSION_H

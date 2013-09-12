/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef JMEDIARECORDER_H
#define JMEDIARECORDER_H

#include <qobject.h>
#include <QtCore/private/qjni_p.h>
#include <qsize.h>

QT_BEGIN_NAMESPACE

class JCamera;

class JMediaRecorder : public QObject, public QJNIObjectPrivate
{
    Q_OBJECT
public:
    enum AudioEncoder {
        DefaultAudioEncoder = 0,
        AMR_NB_Encoder = 1,
        AMR_WB_Encoder = 2,
        AAC = 3
    };

    enum AudioSource {
        DefaultAudioSource = 0,
        Mic = 1,
        VoiceUplink = 2,
        VoiceDownlink = 3,
        VoiceCall = 4,
        Camcorder = 5,
        VoiceRecognition = 6
    };

    enum VideoEncoder {
        DefaultVideoEncoder = 0,
        H263 = 1,
        H264 = 2,
        MPEG_4_SP = 3
    };

    enum VideoSource {
        DefaultVideoSource = 0,
        Camera = 1
    };

    enum OutputFormat {
        DefaultOutputFormat = 0,
        THREE_GPP = 1,
        MPEG_4 = 2,
        AMR_NB_Format = 3,
        AMR_WB_Format = 4
    };

    JMediaRecorder();
    ~JMediaRecorder();

    void release();
    bool prepare();
    void reset();

    bool start();
    void stop();

    void setAudioChannels(int numChannels);
    void setAudioEncoder(AudioEncoder encoder);
    void setAudioEncodingBitRate(int bitRate);
    void setAudioSamplingRate(int samplingRate);
    void setAudioSource(AudioSource source);

    void setCamera(JCamera *camera);
    void setVideoEncoder(VideoEncoder encoder);
    void setVideoEncodingBitRate(int bitRate);
    void setVideoFrameRate(int rate);
    void setVideoSize(const QSize &size);
    void setVideoSource(VideoSource source);

    void setOrientationHint(int degrees);

    void setOutputFormat(OutputFormat format);
    void setOutputFile(const QString &path);

    static bool initJNI(JNIEnv *env);

Q_SIGNALS:
    void error(int what, int extra);
    void info(int what, int extra);

private:
    jlong m_id;
};

QT_END_NAMESPACE

#endif // JMEDIARECORDER_H

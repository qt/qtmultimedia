/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QOPENSLESENGINE_H
#define QOPENSLESENGINE_H

#include <qglobal.h>
#include <qaudio.h>
#include <qlist.h>
#include <qaudioformat.h>
#include <SLES/OpenSLES.h>

QT_BEGIN_NAMESPACE

class QOpenSLESEngine
{
public:
    enum OutputValue { FramesPerBuffer, SampleRate };

    QOpenSLESEngine();
    ~QOpenSLESEngine();

    static QOpenSLESEngine *instance();

    SLEngineItf slEngine() const { return m_engine; }

    static SLDataFormat_PCM audioFormatToSLFormatPCM(const QAudioFormat &format);

    QByteArray defaultDevice(QAudio::Mode mode) const;
    QList<QByteArray> availableDevices(QAudio::Mode mode) const;
    QList<int> supportedChannelCounts(QAudio::Mode mode) const;
    QList<int> supportedSampleRates(QAudio::Mode mode) const;

    static int getOutputValue(OutputValue type, int defaultValue = 0);
    static int getDefaultBufferSize(const QAudioFormat &format);
    static int getLowLatencyBufferSize(const QAudioFormat &format);
    static bool supportsLowLatency();
    static bool printDebugInfo();

private:
    void checkSupportedInputFormats();
    bool inputFormatIsSupported(SLDataFormat_PCM format);

    SLObjectItf m_engineObject;
    SLEngineItf m_engine;

    QList<int> m_supportedInputChannelCounts;
    QList<int> m_supportedInputSampleRates;
    bool m_checkedInputFormats;
};

QT_END_NAMESPACE

#endif // QOPENSLESENGINE_H

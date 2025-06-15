// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERAUDIOOUTPUT_P_H
#define QGSTREAMERAUDIOOUTPUT_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/private/qexpected_p.h>
#include <QtMultimedia/private/qplatformaudiooutput_p.h>

#include <common/qgst_p.h>

QT_BEGIN_NAMESPACE

class QAudioDevice;

class QGstreamerAudioOutput : public QObject, public QPlatformAudioOutput
{
public:
    static q23::expected<QPlatformAudioOutput *, QString> create(QAudioOutput *parent);
    ~QGstreamerAudioOutput() override;

    void setAudioDevice(const QAudioDevice &) override;
    void setVolume(float) override;
    void setMuted(bool) override;
    void setAsync(bool);

    QGstElement gstElement() const { return m_audioOutputBin; }

private:
    explicit QGstreamerAudioOutput(QAudioOutput *parent);

    QGstElement createGstElement();

    QAudioDevice m_audioDevice;

    // Gst elements
    QGstBin m_audioOutputBin;

    QGstElement m_audioQueue;
    QGstElement m_audioConvert;
    QGstElement m_audioResample;
    QGstElement m_audioVolume;
    QGstElement m_audioSink;

    bool m_sinkIsAsync = true;
};

QT_END_NAMESPACE

#endif

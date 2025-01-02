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

#include <private/qtmultimediaglobal_p.h>
#include <private/qmultimediautils_p.h>
#include <qaudiodevice.h>

#include <QtCore/qobject.h>

#include <qgst_p.h>
#include <qgstpipeline_p.h>
#include <private/qplatformaudiooutput_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerMessage;
class QAudioDevice;

class Q_MULTIMEDIA_EXPORT QGstreamerAudioOutput : public QObject, public QPlatformAudioOutput
{
    Q_OBJECT

public:
    static QMaybe<QPlatformAudioOutput *> create(QAudioOutput *parent);
    ~QGstreamerAudioOutput();

    void setAudioDevice(const QAudioDevice &) override;
    void setVolume(float volume) override;
    void setMuted(bool muted) override;

    void setPipeline(const QGstPipeline &pipeline);

    QGstElement gstElement() const { return gstAudioOutput; }

Q_SIGNALS:
    void mutedChanged(bool);
    void volumeChanged(int);

private:
    QGstreamerAudioOutput(QGstElement audioconvert, QGstElement audioresample, QGstElement volume,
                          QGstElement autoaudiosink, QAudioOutput *parent);

    QAudioDevice m_audioOutput;

    // Gst elements
    QGstPipeline gstPipeline;
    QGstBin gstAudioOutput;

    QGstElement audioQueue;
    QGstElement audioConvert;
    QGstElement audioResample;
    QGstElement audioVolume;
    QGstElement audioSink;
};

QT_END_NAMESPACE

#endif

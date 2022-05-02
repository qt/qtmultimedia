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
#include <qaudiodevice.h>

#include <QtCore/qobject.h>

#include <private/qgst_p.h>
#include <private/qgstpipeline_p.h>
#include <private/qplatformaudiooutput_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerMessage;
class QAudioDevice;

class Q_MULTIMEDIA_EXPORT QGstreamerAudioOutput : public QObject, public QPlatformAudioOutput
{
    Q_OBJECT

public:
    QGstreamerAudioOutput(QAudioOutput *parent);
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

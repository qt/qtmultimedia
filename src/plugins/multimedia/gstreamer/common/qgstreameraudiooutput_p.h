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
#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtMultimedia/private/qplatformaudiooutput_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/qaudiodevice.h>

#include <common/qgst_p.h>
#include <common/qgstpipeline_p.h>

QT_BEGIN_NAMESPACE

class QAudioDevice;

class QGstreamerAudioOutput : public QObject, public QPlatformAudioOutput
{
public:
    static QMaybe<QPlatformAudioOutput *> create(QAudioOutput *parent);
    ~QGstreamerAudioOutput();

    void setAudioDevice(const QAudioDevice &) override;
    void setVolume(float) override;
    void setMuted(bool) override;

    QGstElement gstElement() const { return gstAudioOutput; }

private:
    explicit QGstreamerAudioOutput(QAudioOutput *parent);

    QGstElement createGstElement();

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

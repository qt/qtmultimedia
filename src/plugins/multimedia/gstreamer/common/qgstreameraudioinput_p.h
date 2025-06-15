// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERAUDIOINPUT_P_H
#define QGSTREAMERAUDIOINPUT_P_H

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
#include <QtMultimedia/private/qplatformaudioinput_p.h>

#include <common/qgst_p.h>

QT_BEGIN_NAMESPACE

class QAudioDevice;

class QGstreamerAudioInput : public QObject, public QPlatformAudioInput
{
public:
    static q23::expected<QPlatformAudioInput *, QString> create(QAudioInput *parent);
    ~QGstreamerAudioInput() override;

    void setAudioDevice(const QAudioDevice &) override;
    void setVolume(float) override;
    void setMuted(bool) override;

    QGstElement gstElement() const { return m_audioInputBin; }

private:
    explicit QGstreamerAudioInput(QAudioInput *parent);

    QGstElement createGstElement();

    QAudioDevice m_audioDevice;

    // Gst elements
    QGstBin m_audioInputBin;

    QGstElement m_audioSrc;
    QGstElement m_audioVolume;
};

QT_END_NAMESPACE

#endif

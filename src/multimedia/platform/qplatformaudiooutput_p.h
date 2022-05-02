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
#ifndef QPLATFORMAUDIOOUTPUT_H
#define QPLATFORMAUDIOOUTPUT_H

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

QT_BEGIN_NAMESPACE

class QAudioOutput;

class Q_MULTIMEDIA_EXPORT QPlatformAudioOutput
{
public:
    QPlatformAudioOutput(QAudioOutput *qq) : q(qq) {}
    virtual ~QPlatformAudioOutput() {}

    virtual void setAudioDevice(const QAudioDevice &/*device*/) {}
    virtual void setMuted(bool /*muted*/) {}
    virtual void setVolume(float /*volume*/) {}

    QAudioOutput *q = nullptr;
    QAudioDevice device;
    float volume = 1.;
    bool muted = false;
    std::function<void()>  disconnectFunction;
};

QT_END_NAMESPACE


#endif // QPLATFORMAUDIOOUTPUT_H

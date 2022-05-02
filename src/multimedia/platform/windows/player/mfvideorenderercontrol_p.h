/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
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

#ifndef MFVIDEORENDERERCONTROL_H
#define MFVIDEORENDERERCONTROL_H

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

#include "qobject.h"
#include <mfapi.h>
#include <mfidl.h>

QT_USE_NAMESPACE

class EVRCustomPresenterActivate;

QT_BEGIN_NAMESPACE
class QVideoSink;
QT_END_NAMESPACE

class MFVideoRendererControl : public QObject
{
    Q_OBJECT
public:
    MFVideoRendererControl(QObject *parent = 0);
    ~MFVideoRendererControl();

    QVideoSink *sink() const;
    void setSink(QVideoSink *surface);

    IMFActivate* createActivate();
    void releaseActivate();

protected:
    void customEvent(QEvent *event) override;

private Q_SLOTS:
    void present();

private:
    void clear();

    QVideoSink *m_sink = nullptr;
    IMFActivate *m_currentActivate = nullptr;
    IMFSampleGrabberSinkCallback *m_callback = nullptr;

    EVRCustomPresenterActivate *m_presenterActivate = nullptr;
};

#endif

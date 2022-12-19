// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <qobject.h>
#include <qpointer.h>
#include <qrect.h>
#include <mfapi.h>
#include <mfidl.h>

QT_BEGIN_NAMESPACE
class EVRCustomPresenterActivate;
class QVideoSink;

class MFVideoRendererControl : public QObject
{
    Q_OBJECT
public:
    MFVideoRendererControl(QObject *parent = 0);
    ~MFVideoRendererControl();

    QVideoSink *sink() const;
    void setSink(QVideoSink *surface);

    void setCropRect(QRect cropRect);

    IMFActivate* createActivate();
    void releaseActivate();

protected:
    void customEvent(QEvent *event) override;

private Q_SLOTS:
    void present();

private:
    void clear();

    QPointer<QVideoSink> m_sink;
    IMFActivate *m_currentActivate = nullptr;
    IMFSampleGrabberSinkCallback *m_callback = nullptr;

    EVRCustomPresenterActivate *m_presenterActivate = nullptr;
};

QT_END_NAMESPACE

#endif

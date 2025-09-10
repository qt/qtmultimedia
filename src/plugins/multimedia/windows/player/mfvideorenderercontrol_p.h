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
#include <mfobjects.h>
#include <QtCore/private/qcomptr_p.h>

QT_BEGIN_NAMESPACE
class EVRCustomPresenterActivate;
class QVideoSink;

class MFVideoRendererControl : public QObject
{
public:
    MFVideoRendererControl(QObject *parent = 0);
    ~MFVideoRendererControl();

    void setSink(QVideoSink *surface);
    void setCropRect(const QRect &cropRect);

    IMFActivate* createActivate();
    void releaseActivate();

private:
    QPointer<QVideoSink> m_sink;
    ComPtr<IMFActivate> m_currentActivate;
    ComPtr<EVRCustomPresenterActivate> m_presenterActivate;
};

QT_END_NAMESPACE

#endif

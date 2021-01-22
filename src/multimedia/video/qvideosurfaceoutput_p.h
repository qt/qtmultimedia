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

#ifndef QVIDEOSURFACEOUTPUT_P_H
#define QVIDEOSURFACEOUTPUT_P_H

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

#include <qmediabindableinterface.h>

#include <QtCore/qsharedpointer.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE


class QAbstractVideoSurface;
class QVideoRendererControl;

class QVideoSurfaceOutput : public QObject, public QMediaBindableInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaBindableInterface)
public:
    QVideoSurfaceOutput(QObject *parent = nullptr);
    ~QVideoSurfaceOutput();

    QMediaObject *mediaObject() const override;

    void setVideoSurface(QAbstractVideoSurface *surface);

protected:
    bool setMediaObject(QMediaObject *object) override;

private:
    QPointer<QAbstractVideoSurface> m_surface;
    QPointer<QVideoRendererControl> m_control;
    QPointer<QMediaService> m_service;
    QPointer<QMediaObject> m_object;
};

QT_END_NAMESPACE


#endif

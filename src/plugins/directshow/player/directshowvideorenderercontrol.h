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

#ifndef DIRECTSHOWVIDEORENDERERCONTROL_H
#define DIRECTSHOWVIDEORENDERERCONTROL_H

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <dshow.h>

#include "qvideorenderercontrol.h"

#include <QtMultimedia/private/qtmultimedia-config_p.h>

QT_BEGIN_NAMESPACE

class DirectShowEventLoop;
#if QT_CONFIG(evr)
class EVRCustomPresenter;
#endif

class DirectShowVideoRendererControl : public QVideoRendererControl
{
    Q_OBJECT
public:
    DirectShowVideoRendererControl(DirectShowEventLoop *loop, QObject *parent = nullptr);
    ~DirectShowVideoRendererControl() override;

    QAbstractVideoSurface *surface() const override;
    void setSurface(QAbstractVideoSurface *surface) override;

    IBaseFilter *filter();

Q_SIGNALS:
    void filterChanged();
    void positionChanged(qint64 position);

private:
    DirectShowEventLoop *m_loop;
    QAbstractVideoSurface *m_surface = nullptr;
    IBaseFilter *m_filter = nullptr;
#if QT_CONFIG(evr)
    EVRCustomPresenter *m_evrPresenter = nullptr;
#endif
};

QT_END_NAMESPACE

#endif

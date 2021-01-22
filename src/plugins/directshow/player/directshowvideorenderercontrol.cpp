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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include "directshowvideorenderercontrol.h"

#include "videosurfacefilter.h"

#if QT_CONFIG(evr)
#include "evrcustompresenter.h"
#endif

#include <qabstractvideosurface.h>

DirectShowVideoRendererControl::DirectShowVideoRendererControl(DirectShowEventLoop *loop, QObject *parent)
    : QVideoRendererControl(parent)
    , m_loop(loop)
{
}

DirectShowVideoRendererControl::~DirectShowVideoRendererControl()
{
#if QT_CONFIG(evr)
    if (m_evrPresenter) {
        m_evrPresenter->setSurface(nullptr);
        m_evrPresenter->Release();
    }
#endif
    if (m_filter)
        m_filter->Release();
}

QAbstractVideoSurface *DirectShowVideoRendererControl::surface() const
{
    return m_surface;
}

void DirectShowVideoRendererControl::setSurface(QAbstractVideoSurface *surface)
{
    if (m_surface == surface)
        return;

#if QT_CONFIG(evr)
    if (m_evrPresenter) {
        m_evrPresenter->setSurface(nullptr);
        m_evrPresenter->Release();
        m_evrPresenter = nullptr;
    }
#endif

    if (m_filter) {
        m_filter->Release();
        m_filter = nullptr;
    }

    m_surface = surface;

    if (m_surface) {
#if QT_CONFIG(evr)
        if (!qgetenv("QT_DIRECTSHOW_NO_EVR").toInt()) {
            m_filter = com_new<IBaseFilter>(clsid_EnhancedVideoRenderer);
            m_evrPresenter = new EVRCustomPresenter(m_surface);
            connect(this, &DirectShowVideoRendererControl::positionChanged, m_evrPresenter, &EVRCustomPresenter::positionChanged);
            if (!m_evrPresenter->isValid() || !qt_evr_setCustomPresenter(m_filter, m_evrPresenter)) {
                m_filter->Release();
                m_filter = nullptr;
                m_evrPresenter->Release();
                m_evrPresenter = nullptr;
            }
        }

        if (!m_filter)
#endif
        {
            m_filter = new VideoSurfaceFilter(m_surface, m_loop);
        }
    }

    emit filterChanged();
}

IBaseFilter *DirectShowVideoRendererControl::filter()
{
    return m_filter;
}

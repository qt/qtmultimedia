/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qvideosurfaces_p.h"

QT_BEGIN_NAMESPACE

QVideoSurfaces::QVideoSurfaces(const QVector<QAbstractVideoSurface *> &s, QObject *parent)
    : QAbstractVideoSurface(parent)
    , m_surfaces(s)
{
    for (auto a : s) {
        connect(a, &QAbstractVideoSurface::supportedFormatsChanged, this, [this, a] {
            auto context = property("GLContext").value<QObject *>();
            if (!context)
                setProperty("GLContext", a->property("GLContext"));

            emit supportedFormatsChanged();
        });
    }
}

QVideoSurfaces::~QVideoSurfaces()
{
}

QList<QVideoFrame::PixelFormat> QVideoSurfaces::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const
{
    QList<QVideoFrame::PixelFormat> result;
    QMap<QVideoFrame::PixelFormat, int> formats;
    for (auto &s : m_surfaces) {
        for (auto &p : s->supportedPixelFormats(type)) {
            if (++formats[p] == m_surfaces.size())
                result << p;
        }
    }

    return result;
}

bool QVideoSurfaces::start(const QVideoSurfaceFormat &format)
{
    bool result = true;
    for (auto &s : m_surfaces)
        result &= s->start(format);

    return result && QAbstractVideoSurface::start(format);
}

void QVideoSurfaces::stop()
{
    for (auto &s : m_surfaces)
        s->stop();

    QAbstractVideoSurface::stop();
}

bool QVideoSurfaces::present(const QVideoFrame &frame)
{
    bool result = true;
    for (auto &s : m_surfaces)
        result &= s->present(frame);

    return result;
}

QT_END_NAMESPACE

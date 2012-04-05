/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEVIDEOOUTPUT_BACKEND_P_H
#define QDECLARATIVEVIDEOOUTPUT_BACKEND_P_H

#include <QtCore/qpointer.h>
#include <QtCore/qsize.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qsgnode.h>

QT_BEGIN_NAMESPACE

class QAbstractVideoSurface;
class QDeclarativeVideoOutput;
class QMediaService;

class QDeclarativeVideoBackend
{
public:
    explicit QDeclarativeVideoBackend(QDeclarativeVideoOutput *parent)
        : q(parent)
    {}

    virtual ~QDeclarativeVideoBackend()
    {}

    virtual bool init(QMediaService *service) = 0;
    virtual void releaseSource() = 0;
    virtual void releaseControl() = 0;
    virtual void itemChange(QQuickItem::ItemChange change,
                            const QQuickItem::ItemChangeData &changeData) = 0;
    virtual QSize nativeSize() const = 0;
    virtual void updateGeometry() = 0;
    virtual QSGNode *updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *data) = 0;
    virtual QAbstractVideoSurface *videoSurface() const = 0;

protected:
    QDeclarativeVideoOutput *q;
    QPointer<QMediaService> m_service;
};

/*
 * Helper - returns true if the given orientation has the same aspect as the default (e.g. 180*n)
 */
namespace {

inline bool qIsDefaultAspect(int o)
{
    return (o % 180) == 0;
}

/*
 * Return the orientation normalized to 0-359
 */
inline int qNormalizedOrientation(int o)
{
    // Negative orientations give negative results
    int o2 = o % 360;
    if (o2 < 0)
        o2 += 360;
    return o2;
}

}

QT_END_NAMESPACE

#endif

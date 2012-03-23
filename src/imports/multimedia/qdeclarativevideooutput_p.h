/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVEVIDEOOUTPUT_P_H
#define QDECLARATIVEVIDEOOUTPUT_P_H

#include <QtCore/QRectF>

#include <QtQuick/QQuickItem>

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qmediaobject.h>

#include <QtCore/qsharedpointer.h>
#include <QtCore/qmutex.h>

#include <private/qsgvideonode_p.h>


#include "qsgvideonode_i420.h"
#include "qsgvideonode_rgb.h"


QT_BEGIN_NAMESPACE

class QSGVideoItemSurface;
class QVideoRendererControl;
class QMediaService;
class QVideoSurfaceFormat;

class QDeclarativeVideoOutput : public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(QDeclarativeVideoOutput)
    Q_PROPERTY(QObject* source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(QRectF sourceRect READ sourceRect NOTIFY sourceRectChanged)
    Q_PROPERTY(QRectF contentRect READ contentRect NOTIFY contentRectChanged)
    Q_ENUMS(FillMode)

public:
    enum FillMode
    {
        Stretch            = Qt::IgnoreAspectRatio,
        PreserveAspectFit  = Qt::KeepAspectRatio,
        PreserveAspectCrop = Qt::KeepAspectRatioByExpanding
    };

    QDeclarativeVideoOutput(QQuickItem *parent = 0);
    ~QDeclarativeVideoOutput();

    QObject *source() const { return m_source.data(); }
    void setSource(QObject *source);

    FillMode fillMode() const;
    void setFillMode(FillMode mode);

    int orientation() const;
    void setOrientation(int);

    QRectF sourceRect() const;
    QRectF contentRect() const;

    Q_INVOKABLE QPointF mapPointToItem(const QPointF &point) const;
    Q_INVOKABLE QRectF mapRectToItem(const QRectF &rectangle) const;
    Q_INVOKABLE QPointF mapNormalizedPointToItem(const QPointF &point) const;
    Q_INVOKABLE QRectF mapNormalizedRectToItem(const QRectF &rectangle) const;
    Q_INVOKABLE QPointF mapPointToSource(const QPointF &point) const;
    Q_INVOKABLE QRectF mapRectToSource(const QRectF &rectangle) const;
    Q_INVOKABLE QPointF mapPointToSourceNormalized(const QPointF &point) const;
    Q_INVOKABLE QRectF mapRectToSourceNormalized(const QRectF &rectangle) const;

Q_SIGNALS:
    void sourceChanged();
    void fillModeChanged(QDeclarativeVideoOutput::FillMode);
    void orientationChanged();
    void sourceRectChanged();
    void contentRectChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

private Q_SLOTS:
    void _q_updateMediaObject();
    void _q_updateNativeSize(const QVideoSurfaceFormat&);
    void _q_updateGeometry();

private:
    enum SourceType {
        NoSource,
        MediaObjectSource,
        VideoSurfaceSource
    };

    void present(const QVideoFrame &frame);
    void stop();

    friend class QSGVideoItemSurface;

    SourceType m_sourceType;

    QWeakPointer<QObject> m_source;
    QWeakPointer<QMediaObject> m_mediaObject;
    QWeakPointer<QMediaService> m_service;
    QWeakPointer<QVideoRendererControl> m_rendererControl;

    QList<QSGVideoNodeFactoryInterface*> m_videoNodeFactories;
    QSGVideoItemSurface *m_surface;
    QVideoFrame m_frame;
    FillMode m_fillMode;
    QSize m_nativeSize;

    QSGVideoNodeFactory_I420 m_i420Factory;
    QSGVideoNodeFactory_RGB m_rgbFactory;

    bool m_geometryDirty;
    QRectF m_lastSize;      // Cache of last size to avoid recalculating geometry
    QRectF m_renderedRect;  // Destination pixel coordinates, clipped
    QRectF m_contentRect;   // Destination pixel coordinates, unclipped
    QRectF m_sourceTextureRect;    // Source texture coordinates
    int m_orientation;

    QMutex m_frameMutex;
};

QT_END_NAMESPACE

#endif // QDECLARATIVEVIDEOOUTPUT_H

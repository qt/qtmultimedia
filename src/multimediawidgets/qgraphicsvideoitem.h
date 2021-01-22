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

#ifndef QGRAPHICSVIDEOITEM_H
#define QGRAPHICSVIDEOITEM_H

#include <QtWidgets/qgraphicsitem.h>

#include <QtMultimediaWidgets/qvideowidget.h>
#include <QtMultimedia/qmediabindableinterface.h>

#if QT_CONFIG(graphicsview)

QT_BEGIN_NAMESPACE

class QVideoSurfaceFormat;
class QGraphicsVideoItemPrivate;
class Q_MULTIMEDIAWIDGETS_EXPORT QGraphicsVideoItem : public QGraphicsObject, public QMediaBindableInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaBindableInterface)
    Q_PROPERTY(QMediaObject* mediaObject READ mediaObject WRITE setMediaObject)
    Q_PROPERTY(Qt::AspectRatioMode aspectRatioMode READ aspectRatioMode WRITE setAspectRatioMode)
    Q_PROPERTY(QPointF offset READ offset WRITE setOffset)
    Q_PROPERTY(QSizeF size READ size WRITE setSize)
    Q_PROPERTY(QSizeF nativeSize READ nativeSize NOTIFY nativeSizeChanged)
    Q_PROPERTY(QAbstractVideoSurface* videoSurface READ videoSurface CONSTANT)
public:
    explicit QGraphicsVideoItem(QGraphicsItem *parent = nullptr);
    ~QGraphicsVideoItem();

    QMediaObject *mediaObject() const override;
    QAbstractVideoSurface *videoSurface() const;

    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);

    QPointF offset() const;
    void setOffset(const QPointF &offset);

    QSizeF size() const;
    void setSize(const QSizeF &size);

    QSizeF nativeSize() const;

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    enum { Type = 14 };
    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    }
#endif

Q_SIGNALS:
    void nativeSizeChanged(const QSizeF &size);

protected:
    void timerEvent(QTimerEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    bool setMediaObject(QMediaObject *object) override;

    QGraphicsVideoItemPrivate *d_ptr;

private:
    Q_DECLARE_PRIVATE(QGraphicsVideoItem)
    Q_PRIVATE_SLOT(d_func(), void _q_present())
    Q_PRIVATE_SLOT(d_func(), void _q_updateNativeSize())
    Q_PRIVATE_SLOT(d_func(), void _q_serviceDestroyed())
};

QT_END_NAMESPACE

#endif // QT_CONFIG(graphicsview)

#endif // QGRAPHICSVIDEOITEM_H

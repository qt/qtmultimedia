/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QGRAPHICSVIDEOITEM_H
#define QGRAPHICSVIDEOITEM_H

#include <QtWidgets/qgraphicsitem.h>

#include <QtMultimediaWidgets/qvideowidget.h>

#if QT_CONFIG(graphicsview)

QT_BEGIN_NAMESPACE

class QVideoFrameFormat;
class QGraphicsVideoItemPrivate;
class Q_MULTIMEDIAWIDGETS_EXPORT QGraphicsVideoItem : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(Qt::AspectRatioMode aspectRatioMode READ aspectRatioMode WRITE setAspectRatioMode)
    Q_PROPERTY(QPointF offset READ offset WRITE setOffset)
    Q_PROPERTY(QSizeF size READ size WRITE setSize)
    Q_PROPERTY(QSizeF nativeSize READ nativeSize NOTIFY nativeSizeChanged)
    Q_PROPERTY(QVideoSink* videoSink READ videoSink CONSTANT)
public:
    explicit QGraphicsVideoItem(QGraphicsItem *parent = nullptr);
    ~QGraphicsVideoItem();

    Q_INVOKABLE QVideoSink *videoSink() const;

    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);

    QPointF offset() const;
    void setOffset(const QPointF &offset);

    QSizeF size() const;
    void setSize(const QSizeF &size);

    QSizeF nativeSize() const;

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    enum { Type = 14 };
    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    }

Q_SIGNALS:
    void nativeSizeChanged(const QSizeF &size);

protected:
    void timerEvent(QTimerEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    QGraphicsVideoItemPrivate *d_ptr;

private:
    Q_DECLARE_PRIVATE(QGraphicsVideoItem)
    Q_PRIVATE_SLOT(d_func(), void _q_present(const QVideoFrame &))
};

QT_END_NAMESPACE

#endif // QT_CONFIG(graphicsview)

#endif // QGRAPHICSVIDEOITEM_H

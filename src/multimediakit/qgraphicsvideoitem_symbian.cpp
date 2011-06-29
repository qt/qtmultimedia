/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/qglobal.h>

#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtGui/QApplication>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsView>

#include "qgraphicsvideoitem.h"

#include <qmediaobject.h>
#include <qmediaservice.h>
#include <qvideowidgetcontrol.h>

Q_DECLARE_METATYPE(WId)

static const QEvent::Type UpdateViewportTransparencyEvent =
    static_cast<QEvent::Type>(QEvent::registerEventType());

QT_BEGIN_NAMESPACE

class QGraphicsVideoItemPrivate : public QObject
{
    Q_OBJECT

public:
    QGraphicsVideoItemPrivate(QGraphicsVideoItem *parent);
    ~QGraphicsVideoItemPrivate();
    QMediaObject *mediaObject() const;
    bool setMediaObject(QMediaObject *mediaObject);
    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);
    QPointF offset() const;
    void setOffset(const QPointF &offset);
    QSizeF size() const;
    void setSize(const QSizeF &size);
    QRectF rect() const;
    QRectF boundingRect() const;
    QSize nativeSize() const;
    void setCurrentView(QGraphicsView *view);
    void setVisible(bool visible);
    void setZValue(int zValue);
    void setTransform(const QTransform &transform);
    void setWithinViewBounds(bool within);

    bool eventFilter(QObject *watched, QEvent *event);
    void customEvent(QEvent *event);

    void _q_present();
    void _q_updateNativeSize();
    void _q_serviceDestroyed();
    void _q_mediaObjectDestroyed();

public slots:
    void updateWidgetOrdinalPosition();
    void updateItemAncestors();

private:
    void clearService();
    QWidget *videoWidget() const;
    void updateGeometry();
    void updateViewportAncestorEventFilters();
    void updateWidgetVisibility();
    void updateTopWinId();

private:
    QGraphicsVideoItem *q_ptr;
    QMediaService *m_service;
    QMediaObject *m_mediaObject;
    QVideoWidgetControl *m_widgetControl;
    QPointer<QGraphicsView> m_currentView;
    QList<QPointer<QObject> > m_viewportAncestors;
    QList<QPointer<QObject> > m_itemAncestors;
    QGraphicsView::ViewportUpdateMode m_savedViewportUpdateMode;
    Qt::AspectRatioMode m_aspectRatioMode;
    QRectF m_rect;
    QRectF m_boundingRect;
    QSize m_nativeSize;
    QPointF m_offset;
    QTransform m_transform;
    bool m_visible;
    bool m_withinViewBounds;
};


QGraphicsVideoItemPrivate::QGraphicsVideoItemPrivate(QGraphicsVideoItem *parent)
:   q_ptr(parent)
,   m_service(0)
,   m_mediaObject(0)
,   m_widgetControl(0)
,   m_savedViewportUpdateMode(QGraphicsView::FullViewportUpdate)
,   m_aspectRatioMode(Qt::KeepAspectRatio)
,   m_rect(0.0, 0.0, 320.0, 240.0)
,   m_visible(false)
,   m_withinViewBounds(false)
{
    qRegisterMetaType<WId>("WId");
    updateItemAncestors();
}

QGraphicsVideoItemPrivate::~QGraphicsVideoItemPrivate()
{
    if (m_widgetControl)
        m_service->releaseControl(m_widgetControl);
    setCurrentView(0);
}

QMediaObject *QGraphicsVideoItemPrivate::mediaObject() const
{
    return m_mediaObject;
}

bool QGraphicsVideoItemPrivate::setMediaObject(QMediaObject *mediaObject)
{
    bool bound = false;
    if (m_mediaObject != mediaObject) {
        clearService();
        m_mediaObject = mediaObject;
        if (m_mediaObject) {
            m_service = m_mediaObject->service();
            if (m_service) {
                connect(m_service, SIGNAL(destroyed()), q_ptr, SLOT(_q_serviceDestroyed()));
                m_widgetControl = qobject_cast<QVideoWidgetControl *>(
                    m_service->requestControl(QVideoWidgetControl_iid));
                if (m_widgetControl) {
                    connect(m_widgetControl, SIGNAL(nativeSizeChanged()), q_ptr, SLOT(_q_updateNativeSize()));
                    m_widgetControl->setAspectRatioMode(Qt::IgnoreAspectRatio);
                    updateGeometry();
                    updateTopWinId();
                    updateWidgetOrdinalPosition();
                    updateWidgetVisibility();
                    bound = true;
                }
            }
        }
    }
    return bound;
}

Qt::AspectRatioMode QGraphicsVideoItemPrivate::aspectRatioMode() const
{
    return m_aspectRatioMode;
}

void QGraphicsVideoItemPrivate::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    if (mode != m_aspectRatioMode) {
        m_aspectRatioMode = mode;
        updateGeometry();
    }
}

QPointF QGraphicsVideoItemPrivate::offset() const
{
    return m_rect.topLeft();
}

void QGraphicsVideoItemPrivate::setOffset(const QPointF &offset)
{
    if (m_offset != offset) {
        m_offset = offset;
        updateGeometry();
    }
}

QSizeF QGraphicsVideoItemPrivate::size() const
{
    return m_rect.size();
}

void QGraphicsVideoItemPrivate::setSize(const QSizeF &size)
{
    if (m_rect.size() != size) {
        m_rect.setSize(size.isValid() ? size : QSizeF(0, 0));
        updateGeometry();
    }
}

QRectF QGraphicsVideoItemPrivate::rect() const
{
    return m_rect;
}

QRectF QGraphicsVideoItemPrivate::boundingRect() const
{
    return m_boundingRect;
}

QSize QGraphicsVideoItemPrivate::nativeSize() const
{
    return m_nativeSize;
}

void QGraphicsVideoItemPrivate::setCurrentView(QGraphicsView *view)
{
    if (m_currentView != view) {
        if (m_currentView)
            m_currentView->setViewportUpdateMode(m_savedViewportUpdateMode);
        m_currentView = view;
        updateTopWinId();
        if (m_currentView) {
            m_savedViewportUpdateMode = m_currentView->viewportUpdateMode();
            m_currentView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
            updateWidgetOrdinalPosition();
            updateGeometry();
        }
        updateViewportAncestorEventFilters();
     }
}

void QGraphicsVideoItemPrivate::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        updateWidgetVisibility();
    }
}

void QGraphicsVideoItemPrivate::setTransform(const QTransform &transform)
{
    if (m_transform != transform) {
        m_transform = transform;
        updateGeometry();
    }
}

void QGraphicsVideoItemPrivate::setWithinViewBounds(bool within)
{
    if (m_withinViewBounds != within) {
        m_withinViewBounds = within;
        updateWidgetVisibility();
    }
}

bool QGraphicsVideoItemPrivate::eventFilter(QObject *watched, QEvent *event)
{
    bool updateViewportAncestorEventFiltersRequired = false;
    bool updateGeometryRequired = false;
    foreach (QPointer<QObject> target, m_viewportAncestors) {
        if (watched == target.data()) {
            switch (event->type()) {
            case QEvent::ParentChange:
                updateViewportAncestorEventFiltersRequired = true;
                break;
            case QEvent::WinIdChange:
                updateViewportAncestorEventFiltersRequired = true;
                updateTopWinId();
                break;
            case QEvent::Move:
            case QEvent::Resize:
                updateGeometryRequired = true;
                break;
            }
        }
    }
    if (updateViewportAncestorEventFiltersRequired)
        updateViewportAncestorEventFilters();
    if (updateGeometryRequired)
        updateGeometry();
    if (watched == m_currentView) {
        switch (event->type()) {
        case QEvent::Show:
            setVisible(true);
            break;
        case QEvent::Hide:
            setVisible(false);
            break;
        }
    }
    return QObject::eventFilter(watched, event);
}

void QGraphicsVideoItemPrivate::customEvent(QEvent *event)
{
    if (event->type() == UpdateViewportTransparencyEvent && m_currentView) {
        m_currentView->window()->setAttribute(Qt::WA_TranslucentBackground);
        m_currentView->window()->update();
    }
    QObject::customEvent(event);
}

void QGraphicsVideoItemPrivate::clearService()
{
    if (m_widgetControl) {
        m_service->releaseControl(m_widgetControl);
        m_widgetControl = 0;
    }
    if (m_service) {
        m_service->disconnect(q_ptr);
        m_service = 0;
    }
}

QWidget *QGraphicsVideoItemPrivate::videoWidget() const
{
    return m_widgetControl ? m_widgetControl->videoWidget() : 0;
}

void QGraphicsVideoItemPrivate::updateViewportAncestorEventFilters()
{
    // In order to determine when the absolute screen position of the item
    // changes, we need to receive move events sent to m_currentView
    // or any of its ancestors.
    foreach (QPointer<QObject> target, m_viewportAncestors)
        if (target)
            target->removeEventFilter(this);
    m_viewportAncestors.clear();
    QObject *target = m_currentView;
    while (target) {
        target->installEventFilter(this);
        m_viewportAncestors.append(target);
        target = target->parent();
    }
}

void QGraphicsVideoItemPrivate::updateItemAncestors()
{
    // We need to monitor the ancestors of this item to check for zOrder
    // changes and reparenting, both of which influence the stacking order
    // of this item and so require changes to the backend window ordinal position.
    foreach (QPointer<QObject> target, m_itemAncestors) {
        if (target) {
            disconnect(target, SIGNAL(zChanged()), this, SLOT(updateWidgetOrdinalPosition()));
            disconnect(target, SIGNAL(parentChanged()), this, SLOT(updateItemAncestors()));
            disconnect(target, SIGNAL(parentChanged()), this, SLOT(updateWidgetOrdinalPosition()));
        }
    }
    m_itemAncestors.clear();
    QGraphicsItem *item = q_ptr;
    while (item) {
        if (QGraphicsObject *object = item->toGraphicsObject()) {
            connect(object, SIGNAL(zChanged()), this, SLOT(updateWidgetOrdinalPosition()));
            connect(object, SIGNAL(parentChanged()), this, SLOT(updateItemAncestors()));
            connect(object, SIGNAL(parentChanged()), this, SLOT(updateWidgetOrdinalPosition()));
            m_itemAncestors.append(object);
        }
        item = item->parentItem();
    }
}

void QGraphicsVideoItemPrivate::updateGeometry()
{
    q_ptr->prepareGeometryChange();
    QSizeF videoSize;
    if (m_nativeSize.isEmpty()) {
        videoSize = m_rect.size();
    } else if (m_aspectRatioMode == Qt::IgnoreAspectRatio) {
        videoSize = m_rect.size();
    } else {
        // KeepAspectRatio or KeepAspectRatioByExpanding
        videoSize = m_nativeSize;
        videoSize.scale(m_rect.size(), m_aspectRatioMode);
    }
    QRectF displayRect(QPointF(0, 0), videoSize);
    displayRect.moveCenter(m_rect.center());
    m_boundingRect = displayRect.intersected(m_rect);
    if (QWidget *widget = videoWidget()) {
        QRect widgetGeometry;
        QRect extent;
        if (m_currentView) {
            const QRectF viewRectF = m_transform.mapRect(displayRect);
            const QRect viewRect(viewRectF.topLeft().toPoint(), viewRectF.size().toSize());
            // Without this, a line of transparent pixels is visible round the edge of the
            // item.  This is probably down to an error in conversion between scene and
            // screen coordinates, but the root cause has not yet been tracked down.
            static const QPoint positionFudgeFactor(-1, -1);
            static const QSize sizeFudgeFactor(4, 4);
            const QRect videoGeometry(m_currentView->mapToGlobal(viewRect.topLeft()) + positionFudgeFactor,
                                      viewRect.size() + sizeFudgeFactor);
            QRect viewportGeometry = QRect(m_currentView->viewport()->mapToGlobal(QPoint(0, 0)),
                                           m_currentView->viewport()->size());
            widgetGeometry = videoGeometry.intersected(viewportGeometry);
            extent = QRect(videoGeometry.topLeft() - widgetGeometry.topLeft(),
                           videoGeometry.size());
        }
        setWithinViewBounds(!widgetGeometry.size().isEmpty());
        widget->setGeometry(widgetGeometry);
        m_widgetControl->setProperty("extentRect", QVariant::fromValue<QRect>(extent));
        const qreal angle = m_transform.map(QLineF(0, 0, 1, 0)).angle();
        m_widgetControl->setProperty("rotation", QVariant::fromValue<qreal>(angle));
    }
}

void QGraphicsVideoItemPrivate::updateWidgetVisibility()
{
    if (QWidget *widget = videoWidget())
        widget->setVisible(m_visible && m_withinViewBounds);
}

void QGraphicsVideoItemPrivate::updateTopWinId()
{
    if (m_widgetControl) {
        WId topWinId = m_currentView ? m_currentView->effectiveWinId() : 0;
        // Set custom property
        m_widgetControl->setProperty("topWinId", QVariant::fromValue<WId>(topWinId));
    }
}

void QGraphicsVideoItemPrivate::updateWidgetOrdinalPosition()
{
    if (m_currentView) {
        QGraphicsScene *scene = m_currentView->scene();
        const QGraphicsScene::ItemIndexMethod indexMethod = scene->itemIndexMethod();
        scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
        const QList<QGraphicsItem*> items = m_currentView->items();
        QList<QGraphicsVideoItem*> graphicsVideoItems;
        foreach (QGraphicsItem *item, items)
            if (QGraphicsVideoItem *x = qobject_cast<QGraphicsVideoItem *>(item->toGraphicsObject()))
                graphicsVideoItems.append(x);
        int ordinalPosition = 1;
        foreach (QGraphicsVideoItem *item, graphicsVideoItems)
            if (QVideoWidgetControl *widgetControl = item->d_ptr->m_widgetControl)
                widgetControl->setProperty("ordinalPosition", ordinalPosition++);
        scene->setItemIndexMethod(indexMethod);
    }
}

void QGraphicsVideoItemPrivate::_q_present()
{
    // Not required for this implementation of QGraphicsVideoItem
}

void QGraphicsVideoItemPrivate::_q_updateNativeSize()
{
    const QSize size = m_widgetControl ? m_widgetControl->property("nativeSize").value<QSize>() : QSize();
    if (!size.isEmpty() && m_nativeSize != size) {
        m_nativeSize = size;
        updateGeometry();
        emit q_ptr->nativeSizeChanged(m_nativeSize);
    }
}

void QGraphicsVideoItemPrivate::_q_serviceDestroyed()
{
    m_widgetControl = 0;
    m_service = 0;
}

void QGraphicsVideoItemPrivate::_q_mediaObjectDestroyed()
{
    m_mediaObject = 0;
    clearService();
}

QGraphicsVideoItem::QGraphicsVideoItem(QGraphicsItem *parent)
:   QGraphicsObject(parent)
,   d_ptr(new QGraphicsVideoItemPrivate(this))
{
    setCacheMode(NoCache);
    setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
}

QGraphicsVideoItem::~QGraphicsVideoItem()
{
    delete d_ptr;
}

QMediaObject *QGraphicsVideoItem::mediaObject() const
{
    return d_func()->mediaObject();
}

bool QGraphicsVideoItem::setMediaObject(QMediaObject *object)
{
    return d_func()->setMediaObject(object);
}

Qt::AspectRatioMode QGraphicsVideoItem::aspectRatioMode() const
{
    return d_func()->aspectRatioMode();
}

void QGraphicsVideoItem::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    d_func()->setAspectRatioMode(mode);
}

QPointF QGraphicsVideoItem::offset() const
{
    return d_func()->offset();
}

void QGraphicsVideoItem::setOffset(const QPointF &offset)
{
    d_func()->setOffset(offset);
}

QSizeF QGraphicsVideoItem::size() const
{
    return d_func()->size();
}

void QGraphicsVideoItem::setSize(const QSizeF &size)
{
    d_func()->setSize(size);
}

QSizeF QGraphicsVideoItem::nativeSize() const
{
    return d_func()->nativeSize();
}

QRectF QGraphicsVideoItem::boundingRect() const
{
    return d_func()->boundingRect();
}

void QGraphicsVideoItem::paint(
        QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_D(QGraphicsVideoItem);
    QGraphicsView *view = 0;
    if (scene() && !scene()->views().isEmpty())
        view = scene()->views().first();
    d->setCurrentView(view);
    d->setTransform(painter->combinedTransform());
    if (widget && !widget->window()->testAttribute(Qt::WA_TranslucentBackground)) {
        // On Symbian, setting Qt::WA_TranslucentBackground can cause the
        // current window surface to be replaced.  Because of this, it cannot
        // safely be changed from the context of the viewport paintEvent(), so we
        // queue a custom event to set the attribute.
        QEvent *event = new QEvent(UpdateViewportTransparencyEvent);
        QCoreApplication::instance()->postEvent(d, event);
    }
    const QPainter::CompositionMode oldCompositionMode = painter->compositionMode();
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->fillRect(d->boundingRect(), Qt::transparent);
    painter->setCompositionMode(oldCompositionMode);
}

QVariant QGraphicsVideoItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    Q_D(QGraphicsVideoItem);
    switch (change) {
    case ItemScenePositionHasChanged:
        update(boundingRect());
        break;
    case ItemVisibleChange:
        d->setVisible(value.toBool());
        break;
    case ItemZValueHasChanged:
        d->updateWidgetOrdinalPosition();
        break;
    default:
        break;
    }
    return QGraphicsItem::itemChange(change, value);
}

void QGraphicsVideoItem::timerEvent(QTimerEvent *event)
{
    QGraphicsObject::timerEvent(event);
}

#include "qgraphicsvideoitem_symbian.moc"
#include "moc_qgraphicsvideoitem.cpp"

QT_END_NAMESPACE

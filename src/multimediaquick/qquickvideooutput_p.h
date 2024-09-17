// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKVIDEOOUTPUT_P_H
#define QQUICKVIDEOOUTPUT_P_H

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

#include <QtCore/qrect.h>
#include <QtCore/qsharedpointer.h>
#include <QtQuick/qquickitem.h>
#include <QtCore/qpointer.h>
#include <QtCore/qmutex.h>

#include <private/qtmultimediaquickglobal_p.h>
#include <qvideoframe.h>
#include <qvideoframeformat.h>
#include <qvideosink.h>

QT_BEGIN_NAMESPACE

class QQuickVideoBackend;
class QVideoOutputOrientationHandler;
class QVideoSink;
class QSGVideoNode;

class QQuickVideoSink : public QVideoSink
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoSink)
public:
    QQuickVideoSink(QObject *parent = nullptr) : QVideoSink(parent)
    {
        connect(this, &QVideoSink::videoFrameChanged, this, &QQuickVideoSink::videoFrameChanged,
                Qt::DirectConnection);
    }

Q_SIGNALS:
    void videoFrameChanged();
};

class Q_MULTIMEDIAQUICK_EXPORT QQuickVideoOutput : public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(QQuickVideoOutput)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(QRectF sourceRect READ sourceRect NOTIFY sourceRectChanged)
    Q_PROPERTY(QRectF contentRect READ contentRect NOTIFY contentRectChanged)
    Q_PROPERTY(QVideoSink* videoSink READ videoSink CONSTANT)
    Q_MOC_INCLUDE(qvideosink.h)
    Q_MOC_INCLUDE(qvideoframe.h)
    QML_NAMED_ELEMENT(VideoOutput)

public:

    enum FillMode
    {
        Stretch            = Qt::IgnoreAspectRatio,
        PreserveAspectFit  = Qt::KeepAspectRatio,
        PreserveAspectCrop = Qt::KeepAspectRatioByExpanding
    };
    Q_ENUM(FillMode)

    QQuickVideoOutput(QQuickItem *parent = 0);
    ~QQuickVideoOutput();

    Q_INVOKABLE QVideoSink *videoSink() const;

    FillMode fillMode() const;
    void setFillMode(FillMode mode);

    int orientation() const;
    void setOrientation(int);

    QRectF sourceRect() const;
    QRectF contentRect() const;

Q_SIGNALS:
    void sourceChanged();
    void fillModeChanged(QQuickVideoOutput::FillMode);
    void orientationChanged();
    void sourceRectChanged();
    void contentRectChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void itemChange(ItemChange change, const ItemChangeData &changeData) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;

private:
    QSize nativeSize() const;
    void updateGeometry();
    QRectF adjustedViewport() const;

    void setFrame(const QVideoFrame &frame);

    void invalidateSceneGraph();

    void initRhiForSink();
    void updateHdr(QSGVideoNode *videoNode);

private Q_SLOTS:
    void _q_newFrame(QSize);
    void _q_updateGeometry();
    void _q_invalidateSceneGraph();
    void _q_sceneGraphInitialized();

private:
    QSize m_nativeSize;

    bool m_geometryDirty = true;
    QRectF m_lastRect;      // Cache of last rect to avoid recalculating geometry
    QRectF m_contentRect;   // Destination pixel coordinates, unclipped
    int m_orientation = 0;
    QtVideo::Rotation m_frameDisplayingRotation = QtVideo::Rotation::None;
    Qt::AspectRatioMode m_aspectRatioMode = Qt::KeepAspectRatio;

    QPointer<QQuickWindow> m_window;
    QVideoSink *m_sink = nullptr;
    QVideoFrameFormat m_videoFormat;

    QList<QVideoFrame> m_videoFrameQueue;
    QVideoFrame m_frame;
    bool m_frameChanged = false;
    QMutex m_frameMutex;
    QRectF m_renderedRect;         // Destination pixel coordinates, clipped
    QRectF m_sourceTextureRect;    // Source texture coordinates
};

QT_END_NAMESPACE

#endif // QQUICKVIDEOOUTPUT_P_H

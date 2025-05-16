// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
#include <QtCore/qpointer.h>
#include <QtCore/qmutex.h>
#include <QtQuick/qquickitem.h>

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/qvideosink.h>
#include <QtMultimediaQuick/private/qtmultimediaquickglobal_p.h>

#include <thread>

QT_BEGIN_NAMESPACE

class QQuickVideoBackend;
class QVideoOutputOrientationHandler;
class QVideoSink;
class QSGVideoNode;
class QVideoFrameTexturePool;
using QVideoFrameTexturePoolWPtr = std::weak_ptr<QVideoFrameTexturePool>;

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

    enum EndOfStreamPolicy
    {
        ClearOutput,
        KeepLastFrame
    };
    Q_ENUM(EndOfStreamPolicy)

    QQuickVideoOutput(QQuickItem *parent = 0);
    ~QQuickVideoOutput() override;

    Q_INVOKABLE QVideoSink *videoSink() const;

    FillMode fillMode() const;
    void setFillMode(FillMode mode);

    int orientation() const;
    void setOrientation(int);

    QRectF sourceRect() const;
    QRectF contentRect() const;

    Q_INVOKABLE void clearOutput();

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

    void initRhiForSink();
    void updateHdr(QSGVideoNode *videoNode);
    void disconnectWindowConnections();

private Q_SLOTS:
    void _q_newFrame(QSize);
    void _q_updateGeometry();

private:
    QSize m_nativeSize;

    bool m_geometryDirty = true;
    QRectF m_lastRect;      // Cache of last rect to avoid recalculating geometry
    QRectF m_contentRect;   // Destination pixel coordinates, unclipped
    int m_orientation = 0;
    bool m_mirrored = false;
    QtVideo::Rotation m_frameDisplayingRotation = QtVideo::Rotation::None;
    Qt::AspectRatioMode m_aspectRatioMode = Qt::KeepAspectRatio;

    QPointer<QQuickWindow> m_window;
    QVideoSink *m_sink = nullptr;
    QVideoFrameFormat m_videoFormat;

    QVideoFrameTexturePoolWPtr m_texturePool;
    QVideoFrame m_frame;
    bool m_frameChanged = false;
    QMutex m_frameMutex;
    QRectF m_renderedRect;         // Destination pixel coordinates, clipped
    QRectF m_sourceTextureRect;    // Source texture coordinates

    EndOfStreamPolicy m_endOfStreamPolicy = ClearOutput;

    struct DestructorGuard
    {
        QMutex m_mutex;
        bool m_isAlive{ true };

        template <typename Functor>
        void runWhileAlive(Functor &&f)
        {
            QMutexLocker lock(&m_mutex);
            if (m_isAlive)
                f();
        }
    };

    std::shared_ptr<DestructorGuard> m_destructorGuard = std::make_shared<DestructorGuard>();

    template <typename Functor>
    auto makeGuardedCall(Functor &&f)
    {
        return [f = std::forward<Functor>(f), guard = m_destructorGuard](auto... params) {
            if (g_signalBackoff)
                std::this_thread::sleep_for(*g_signalBackoff);

            guard->runWhileAlive([&] {
                f(params...);
            });
        };
    }

    // for testing
    static std::optional<std::chrono::nanoseconds> g_signalBackoff;

public:
    static void setSignalBackoff(std::optional<std::chrono::nanoseconds>);
};

QT_END_NAMESPACE

#endif // QQUICKVIDEOOUTPUT_P_H

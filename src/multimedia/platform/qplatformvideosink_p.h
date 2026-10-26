// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMVIDEOSINK_H
#define QPLATFORMVIDEOSINK_H

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

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideosink.h>
#include <QtGui/qwindowdefs.h>
#include <QtCore/private/qglobal_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtGui/rhi/qrhi.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QPlatformVideoSink : public QObject
{
    Q_OBJECT

public:
    ~QPlatformVideoSink() override;

    explicit QPlatformVideoSink(QVideoSink *parent);

    virtual void setRhi(QRhi * /*rhi*/) {}

    QSize nativeSize() const;

    QVideoSink *videoSink() { return m_sink; }

    void setNativeSize(QSize s);

    void setVideoFrame(const QVideoFrame &frame);

    QVideoFrame currentVideoFrame() const;

    void setSubtitleText(const QString &subtitleText);

    QString subtitleText() const;

protected:
    virtual void onVideoFrameChanged(const QVideoFrame &) { }

    QMutex &mutex() const { return m_mutex; }

private:
    QVideoSink *const m_sink = nullptr;
    mutable QMutex m_mutex; // Protects all members below
    QSize m_nativeSize;
    QString m_subtitleText;
    QVideoFrame m_currentVideoFrame;
};

class Q_MULTIMEDIA_EXPORT QRhiVideoSink : public QPlatformVideoSink
{
    Q_OBJECT

public:
    explicit QRhiVideoSink(QVideoSink *sink);
    void setRhi(QRhi *rhi) override final;

    template <typename Functor>
    auto withRhi(Functor &&functor) const
    {
        std::lock_guard guard{ mutex() };
        return functor(m_rhi);
    }

Q_SIGNALS:
    void rhiChanged(QRhi *rhi);

protected:
    virtual bool isBackendSupported(QRhi::Implementation) const { return true; }

private:
    QRhi *m_rhi = nullptr;
};

class Q_MULTIMEDIA_EXPORT QOpenGLRhiVideoSink final : public QRhiVideoSink
{
public:
    using QRhiVideoSink::QRhiVideoSink;

protected:
    bool isBackendSupported(QRhi::Implementation backend) const override;
};

QT_END_NAMESPACE

#endif

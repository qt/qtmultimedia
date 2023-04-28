// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDVIDEOOUTPUT_H
#define QANDROIDVIDEOOUTPUT_H

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

#include <qsize.h>
#include <qmutex.h>
#include <qreadwritelock.h>
#include <private/qabstractvideobuffer_p.h>
#include <qmatrix4x4.h>
#include <qoffscreensurface.h>
#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

class AndroidSurfaceTexture;
class AndroidSurfaceHolder;
class QVideoSink;

class QAndroidVideoOutput : public QObject
{
    Q_OBJECT
public:
    virtual ~QAndroidVideoOutput() { }

    virtual AndroidSurfaceTexture *surfaceTexture() { return 0; }
    virtual AndroidSurfaceHolder *surfaceHolder() { return 0; }

    virtual bool isReady() { return true; }

    virtual void setVideoSize(const QSize &) { }
    virtual void start() { }
    virtual void stop() { }
    virtual void reset() { }
    virtual QSize getVideoSize() const { return QSize(0, 0); }

Q_SIGNALS:
    void readyChanged(bool);

protected:
    QAndroidVideoOutput(QObject *parent) : QObject(parent) { }
};

class QAndroidTextureVideoOutput : public QAndroidVideoOutput
{
    Q_OBJECT
public:
    explicit QAndroidTextureVideoOutput(QVideoSink *sink, QObject *parent = 0);
    ~QAndroidTextureVideoOutput() override;

    QVideoSink *surface() const { return m_sink; }
    bool shouldTextureBeUpdated() const;

    AndroidSurfaceTexture *surfaceTexture() override;

    void setVideoSize(const QSize &) override;
    void stop() override;
    void reset() override;
    QSize getVideoSize() const override { return m_nativeSize; }

    void setSubtitle(const QString &subtitle);
private Q_SLOTS:
    void newFrame(const QVideoFrame &);

private:
    QVideoSink *m_sink = nullptr;
    QSize m_nativeSize;
    bool m_surfaceCreatedWithoutRhi = false;

    std::unique_ptr<class AndroidTextureThread> m_surfaceThread;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QList<QRhiResource *>)
Q_DECLARE_METATYPE(QRhi*)

#endif // QANDROIDVIDEOOUTPUT_H

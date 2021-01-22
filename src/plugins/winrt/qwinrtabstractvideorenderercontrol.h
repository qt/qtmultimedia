/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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

#ifndef QWINRTABSTRACTVIDEORENDERERCONTROL_H
#define QWINRTABSTRACTVIDEORENDERERCONTROL_H

#include <QtMultimedia/QVideoRendererControl>
#include <QtMultimedia/QVideoSurfaceFormat>

#include <qt_windows.h>

struct ID3D11Device;
struct ID3D11Texture2D;

QT_BEGIN_NAMESPACE

class QWinRTAbstractVideoRendererControlPrivate;
class QWinRTAbstractVideoRendererControl : public QVideoRendererControl
{
    Q_OBJECT
public:
    explicit QWinRTAbstractVideoRendererControl(const QSize &size, QObject *parent = nullptr);
    ~QWinRTAbstractVideoRendererControl() override;

    enum BlitMode {
        DirectVideo,
        MediaFoundation
    };

    QAbstractVideoSurface *surface() const override;
    void setSurface(QAbstractVideoSurface *surface) override;

    QSize size() const;
    void setSize(const QSize &size);

    void setScanLineDirection(QVideoSurfaceFormat::Direction direction);

    BlitMode blitMode() const;
    void setBlitMode(BlitMode mode);

    virtual bool render(ID3D11Texture2D *texture) = 0;
    virtual bool dequeueFrame(QVideoFrame *frame);

    static ID3D11Device *d3dDevice();

public slots:
    void setActive(bool active);

protected:
    void shutdown();

private slots:
    void syncAndRender();

private:
    void textureToFrame();
    Q_INVOKABLE void present();

    QScopedPointer<QWinRTAbstractVideoRendererControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTAbstractVideoRendererControl)
};

QT_END_NAMESPACE

#endif // QWINRTABSTRACTVIDEORENDERERCONTROL_H

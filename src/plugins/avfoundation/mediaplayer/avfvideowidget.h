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

#ifndef AVFVIDEOWIDGET_H
#define AVFVIDEOWIDGET_H

#include <QtWidgets/QWidget>

@class AVPlayerLayer;
#if defined(Q_OS_OSX)
@class NSView;
#else
@class UIView;
#endif

QT_BEGIN_NAMESPACE

class AVFVideoWidget : public QWidget
{
public:
    AVFVideoWidget(QWidget *parent);
    virtual ~AVFVideoWidget();

    QSize sizeHint() const override;
    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);
    void setPlayerLayer(AVPlayerLayer *layer);

protected:
    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;

private:
    void updateAspectRatio();
    void updatePlayerLayerBounds(const QSize &size);

    QSize m_nativeSize;
    Qt::AspectRatioMode m_aspectRatioMode;
    AVPlayerLayer *m_playerLayer;
#if defined(Q_OS_OSX)
    NSView *m_nativeView;
#else
    UIView *m_nativeView;
#endif
};

QT_END_NAMESPACE

#endif // AVFVIDEOWIDGET_H

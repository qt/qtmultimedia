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

#ifndef QPLATFORMVIDEOSINK_H
#define QPLATFORMVIDEOSINK_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtGui/qwindowdefs.h>
#include <qvideosink.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QPlatformVideoSink : public QObject
{
    Q_OBJECT

public:

    virtual QVideoSink::GraphicsType graphicsType() const { return QVideoSink::NativeWindow; }
    virtual bool setGraphicsType(QVideoSink::GraphicsType /*type*/) { return false; }

    virtual void setWinId(WId id) = 0;

    virtual void setRhi(QRhi */*rhi*/) {}

    virtual void setDisplayRect(const QRect &rect) = 0;

    virtual void setFullScreen(bool fullScreen) = 0;

    virtual QSize nativeSize() const = 0;

    virtual void setAspectRatioMode(Qt::AspectRatioMode mode) = 0;

    virtual void setBrightness(float brightness) = 0;
    virtual void setContrast(float contrast) = 0;
    virtual void setHue(float hue) = 0;
    virtual void setSaturation(float saturation) = 0;

    QVideoSink *videoSink() { return sink; }

Q_SIGNALS:
    void nativeSizeChanged();

protected:
    explicit QPlatformVideoSink(QVideoSink *parent);
    QVideoSink *sink = nullptr;
};

QT_END_NAMESPACE


#endif

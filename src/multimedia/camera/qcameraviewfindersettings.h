/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCAMERAVIEWFINDERSETTINGS_H
#define QCAMERAVIEWFINDERSETTINGS_H

#include <QtCore/qsharedpointer.h>
#include <QtMultimedia/qtmultimediadefs.h>
#include <QtMultimedia/qvideoframe.h>

QT_BEGIN_NAMESPACE

class QCameraViewfinderSettingsPrivate;

class Q_MULTIMEDIA_EXPORT QCameraViewfinderSettings
{
public:
    QCameraViewfinderSettings();
    QCameraViewfinderSettings(const QCameraViewfinderSettings& other);

    ~QCameraViewfinderSettings();

    QCameraViewfinderSettings& operator=(const QCameraViewfinderSettings &other);
    bool operator==(const QCameraViewfinderSettings &other) const;
    bool operator!=(const QCameraViewfinderSettings &other) const;

    bool isNull() const;

    QSize resolution() const;
    void setResolution(const QSize &);
    inline void setResolution(int width, int height)
    { setResolution(QSize(width, height)); }

    qreal minimumFrameRate() const;
    void setMinimumFrameRate(qreal rate);

    qreal maximumFrameRate() const;
    void setMaximumFrameRate(qreal rate);

    QVideoFrame::PixelFormat pixelFormat() const;
    void setPixelFormat(QVideoFrame::PixelFormat format);

    QSize pixelAspectRatio() const;
    void setPixelAspectRatio(const QSize &ratio);
    inline void setPixelAspectRatio(int horizontal, int vertical)
    { setPixelAspectRatio(QSize(horizontal, vertical)); }

private:
    QSharedDataPointer<QCameraViewfinderSettingsPrivate> d;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QCameraViewfinderSettings)

#endif // QCAMERAVIEWFINDERSETTINGS_H

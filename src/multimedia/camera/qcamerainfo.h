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

#ifndef QCAMERAINFO_H
#define QCAMERAINFO_H

#include <QtMultimedia/qvideoframe.h>
#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

class QCameraFormatPrivate;
class Q_MULTIMEDIA_EXPORT QCameraFormat
{
public:
    QCameraFormat() noexcept;
    QCameraFormat(const QCameraFormat &other) noexcept;
    QCameraFormat &operator=(const QCameraFormat &other) noexcept;
    ~QCameraFormat();

    QVideoFrameFormat::PixelFormat pixelFormat() const noexcept;
    QSize resolution() const noexcept;
    float minFrameRate() const noexcept;
    float maxFrameRate() const noexcept;

    bool isNull() const noexcept { return !d; }

    bool operator==(const QCameraFormat &other) const;
    inline bool operator!=(const QCameraFormat &other) const
    { return !operator==(other); }

private:
    friend class QCameraFormatPrivate;
    QCameraFormat(QCameraFormatPrivate *p);
    QExplicitlySharedDataPointer<QCameraFormatPrivate> d;
};

class QCameraInfoPrivate;
class Q_MULTIMEDIA_EXPORT QCameraInfo
{
    Q_GADGET
    Q_PROPERTY(QByteArray id READ id CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(bool isDefault READ isDefault CONSTANT)
    Q_PROPERTY(Position position READ position CONSTANT)
    Q_ENUMS(Position)
public:
    QCameraInfo();
    QCameraInfo(const QCameraInfo& other);
    QCameraInfo& operator=(const QCameraInfo& other);
    ~QCameraInfo();

    bool operator==(const QCameraInfo &other) const;
    inline bool operator!=(const QCameraInfo &other) const;

    bool isNull() const;

    QByteArray id() const;
    QString description() const;

    // ### Add here and to QAudioDeviceInfo
//    QByteArray groupId() const;
//    QString groupDescription() const;

    bool isDefault() const;

    enum Position
    {
        UnspecifiedPosition,
        BackFace,
        FrontFace
    };

    Position position() const;

    QList<QSize> photoResolutions() const;
    QList<QCameraFormat> videoFormats() const;

    // ### Add zoom and other camera information

private:
    friend class QCameraInfoPrivate;
    QCameraInfo(QCameraInfoPrivate *p);
    QExplicitlySharedDataPointer<QCameraInfoPrivate> d;
};

bool QCameraInfo::operator!=(const QCameraInfo &other) const { return !operator==(other); }

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, const QCameraInfo&);
#endif

QT_END_NAMESPACE

#endif // QCAMERAINFO_H

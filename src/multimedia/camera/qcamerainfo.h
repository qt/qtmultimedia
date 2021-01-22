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

#ifndef QCAMERAINFO_H
#define QCAMERAINFO_H

#include <QtMultimedia/qcamera.h>
#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

class QCameraInfoPrivate;

class Q_MULTIMEDIA_EXPORT QCameraInfo
{
public:
    explicit QCameraInfo(const QByteArray &name = QByteArray());
    explicit QCameraInfo(const QCamera &camera);
    QCameraInfo(const QCameraInfo& other);
    ~QCameraInfo();

    QCameraInfo& operator=(const QCameraInfo& other);
    bool operator==(const QCameraInfo &other) const;
    inline bool operator!=(const QCameraInfo &other) const;

    bool isNull() const;

    QString deviceName() const;
    QString description() const;
    QCamera::Position position() const;
    int orientation() const;

    static QCameraInfo defaultCamera();
    static QList<QCameraInfo> availableCameras(QCamera::Position position = QCamera::UnspecifiedPosition);

private:
    QSharedPointer<QCameraInfoPrivate> d;
};

bool QCameraInfo::operator!=(const QCameraInfo &other) const { return !operator==(other); }

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, const QCameraInfo&);
#endif

QT_END_NAMESPACE

#endif // QCAMERAINFO_H

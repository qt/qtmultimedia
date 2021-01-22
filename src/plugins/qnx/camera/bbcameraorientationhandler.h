/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef BBCAMERAORIENTATIONHANDLER_H
#define BBCAMERAORIENTATIONHANDLER_H

#include <QAbstractNativeEventFilter>
#include <QObject>

QT_BEGIN_NAMESPACE

class BbCameraOrientationHandler : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit BbCameraOrientationHandler(QObject *parent = 0);
    ~BbCameraOrientationHandler();

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

    int orientation() const;

    int viewfinderOrientation() const;

Q_SIGNALS:
    void orientationChanged(int degree);

private:
    int m_orientation;
};

QT_END_NAMESPACE

#endif

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

#ifndef QVIDEOOUTPUTORIENTATIONHANDLER_P_H
#define QVIDEOOUTPUTORIENTATIONHANDLER_P_H

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

#include <qtmultimediaglobal.h>

#include <QObject>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QVideoOutputOrientationHandler : public QObject
{
    Q_OBJECT
public:
    explicit QVideoOutputOrientationHandler(QObject *parent = nullptr);

    int currentOrientation() const;

Q_SIGNALS:
    void orientationChanged(int angle);

private Q_SLOTS:
    void screenOrientationChanged(Qt::ScreenOrientation orientation);

private:
    int m_currentOrientation;
};

QT_END_NAMESPACE


#endif

/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QDECLARATIVECAMERAPREVIEWPROVIDER_H
#define QDECLARATIVECAMERAPREVIEWPROVIDER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick/qquickimageprovider.h>

QT_BEGIN_NAMESPACE

class QDeclarativeCameraPreviewProvider : public QQuickImageProvider
{
public:
    QDeclarativeCameraPreviewProvider();
    ~QDeclarativeCameraPreviewProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize& requestedSize) override;
    static void registerPreview(const QString &id, const QImage &preview);
};

QT_END_NAMESPACE

#endif

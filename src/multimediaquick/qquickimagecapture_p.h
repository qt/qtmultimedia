/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QQUICKIMAGECAPTURE_H
#define QQUICKIMAGECAPTURE_H

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

#include <qcamera.h>
#include <qimagecapture.h>
#include <qmediametadata.h>

#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QUrl;

class QQuickImageCapture : public QImageCapture
{
    Q_OBJECT
    Q_PROPERTY(QString preview READ preview NOTIFY previewChanged)
    QML_NAMED_ELEMENT(ImageCapture)

public:
    QQuickImageCapture(QObject *parent = nullptr);
    ~QQuickImageCapture();

    QString preview() const;

public Q_SLOTS:
    void saveToFile(const QUrl &location) const;

Q_SIGNALS:
    void previewChanged();

private Q_SLOTS:
    void _q_imageCaptured(int, const QImage&);

private:
    QImage m_lastImage;
    QString m_capturedImagePath;
};

QT_END_NAMESPACE

#endif

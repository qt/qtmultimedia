// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <QtMultimedia/qcamera.h>
#include <QtMultimedia/qimagecapture.h>
#include <QtMultimedia/qmediametadata.h>
#include <QtQml/qqml.h>
#include <QtCore/quuid.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QUrl;

class QQuickImageCapture : public QImageCapture
{
    Q_OBJECT
    Q_PROPERTY(QString preview READ preview NOTIFY previewChanged)
    QML_NAMED_ELEMENT(ImageCapture)

public:
    QQuickImageCapture(QObject *parent = nullptr);
    ~QQuickImageCapture() override;

    QString preview() const;

public Q_SLOTS:
    void saveToFile(const QUrl &location) const;

Q_SIGNALS:
    void previewChanged();

private Q_SLOTS:
    void _q_imageCaptured(int, const QImage&);

private:
    QUuid m_instanceId = QUuid::createUuid();
    QImage m_lastImage;
    QString m_capturedImagePath;
};

QT_END_NAMESPACE

#endif

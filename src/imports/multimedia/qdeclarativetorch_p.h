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


#ifndef QDECLARATIVETORCH_P_H
#define QDECLARATIVETORCH_P_H

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

#include <QtCore/QObject>
#include <QtQml/qqml.h>
#include <qcamera.h>
#include <qcameraexposurecontrol.h>
#include <qcameraflashcontrol.h>

QT_BEGIN_NAMESPACE

class QDeclarativeTorch : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int power READ power WRITE setPower NOTIFY powerChanged)

    // Errors?

public:
    explicit QDeclarativeTorch(QObject *parent = 0);
    ~QDeclarativeTorch();

    bool enabled() const;
    void setEnabled(bool enabled);

    int power() const;
    void setPower(int power);

signals:
    void enabledChanged();
    void powerChanged();

private slots:
    void parameterChanged(int parameter);

private:
    QCamera *m_camera;
    QCameraExposureControl *m_exposure;
    QCameraFlashControl *m_flash;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QT_PREPEND_NAMESPACE(QDeclarativeTorch))

#endif // QDECLARATIVETORCH_P_H

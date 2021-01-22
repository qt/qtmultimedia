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

#ifndef QDECLARATIVEMULTIMEDIAGLOBAL_P_H
#define QDECLARATIVEMULTIMEDIAGLOBAL_P_H

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

#include <QtQml/qqml.h>
#include <QtQml/qjsvalue.h>
#include <QtMultimedia/qaudio.h>

QT_BEGIN_NAMESPACE

class QDeclarativeMultimediaGlobal : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QJSValue defaultCamera READ defaultCamera NOTIFY defaultCameraChanged)
    Q_PROPERTY(QJSValue availableCameras READ availableCameras NOTIFY availableCamerasChanged)

    Q_ENUMS(VolumeScale)

public:
    enum VolumeScale {
        LinearVolumeScale = QAudio::LinearVolumeScale,
        CubicVolumeScale = QAudio::CubicVolumeScale,
        LogarithmicVolumeScale = QAudio::LogarithmicVolumeScale,
        DecibelVolumeScale = QAudio::DecibelVolumeScale
    };

    explicit QDeclarativeMultimediaGlobal(QJSEngine *engine, QObject *parent = 0);

    QJSValue defaultCamera() const;
    QJSValue availableCameras() const;

    Q_INVOKABLE qreal convertVolume(qreal volume, VolumeScale from, VolumeScale to) const;

Q_SIGNALS:
    // Unused at the moment. QCameraInfo doesn't notify when cameras are added or removed,
    // but it might change in the future.
    void defaultCameraChanged();
    void availableCamerasChanged();

private:
    QJSEngine *m_engine;
};

QT_END_NAMESPACE

#endif // QDECLARATIVEMULTIMEDIAGLOBAL_P_H

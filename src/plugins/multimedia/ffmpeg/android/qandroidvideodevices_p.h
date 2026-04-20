// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDVIDEODEVICES_H
#define QANDROIDVIDEODEVICES_H

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

#include <QObject>
#include <QJniObject>

#include <QtMultimedia/private/qplatformvideodevices_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(
    QtCameraAvailabilityListener,
    "org/qtproject/qt/android/multimedia/qffmpeg/QtCameraAvailabilityListener");

namespace QFFmpeg {

using QJavaCameraAvailabilityListener = QtJniTypes::QtCameraAvailabilityListener;

class QAndroidVideoDevices : public QPlatformVideoDevices
{
    Q_OBJECT
public:
    QAndroidVideoDevices(QPlatformMediaIntegration *integration);
    ~QAndroidVideoDevices() override;

    using QPlatformVideoDevices::onVideoInputsChanged;

    [[nodiscard]] static bool registerNativeMethods();

protected:
    QList<QCameraDevice> findVideoInputs() const override;

private:
    QJavaCameraAvailabilityListener m_javaCameraAvailabilityListener;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QANDROIDVIDEODEVICES_H

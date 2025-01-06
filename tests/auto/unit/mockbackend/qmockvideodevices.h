// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMOCKVIDEODEVICES_H
#define QMOCKVIDEODEVICES_H

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

#include <private/qplatformvideodevices_p.h>

QT_BEGIN_NAMESPACE

class QPlatformMediaIntegration;

class QMockVideoDevices : public QPlatformVideoDevices
{
public:
    QMockVideoDevices(QPlatformMediaIntegration *mediaIntegration);

    void addNewCamera();

    QList<QCameraDevice> findVideoInputs() const override;

    int getFindVideoInputsInvokeCount() const { return m_findVideoInputsInvokeCount; }

private:
    mutable int m_findVideoInputsInvokeCount = 0;
    QList<QCameraDevice> m_cameraDevices;
};

QT_END_NAMESPACE

#endif // QMOCKVIDEODEVICES_H

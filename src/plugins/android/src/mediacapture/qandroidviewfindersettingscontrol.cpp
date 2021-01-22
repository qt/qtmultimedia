/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Ruslan Baratov
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

#include "qandroidviewfindersettingscontrol.h"
#include "qandroidcamerasession.h"

QT_BEGIN_NAMESPACE

QAndroidViewfinderSettingsControl2::QAndroidViewfinderSettingsControl2(QAndroidCameraSession *session)
    : m_cameraSession(session)
{
}

QList<QCameraViewfinderSettings> QAndroidViewfinderSettingsControl2::supportedViewfinderSettings() const
{
    QList<QCameraViewfinderSettings> viewfinderSettings;

    const QList<QSize> previewSizes = m_cameraSession->getSupportedPreviewSizes();
    const QList<QVideoFrame::PixelFormat> pixelFormats = m_cameraSession->getSupportedPixelFormats();
    const QList<AndroidCamera::FpsRange> fpsRanges = m_cameraSession->getSupportedPreviewFpsRange();

    viewfinderSettings.reserve(previewSizes.size() * pixelFormats.size() * fpsRanges.size());

    for (const QSize& size : previewSizes) {
        for (QVideoFrame::PixelFormat pixelFormat : pixelFormats) {
            for (const AndroidCamera::FpsRange& fpsRange : fpsRanges) {
                QCameraViewfinderSettings s;
                s.setResolution(size);
                s.setPixelAspectRatio(QSize(1, 1));
                s.setPixelFormat(pixelFormat);
                s.setMinimumFrameRate(fpsRange.getMinReal());
                s.setMaximumFrameRate(fpsRange.getMaxReal());
                viewfinderSettings << s;
            }
        }
    }
    return viewfinderSettings;
}

QCameraViewfinderSettings QAndroidViewfinderSettingsControl2::viewfinderSettings() const
{
    return m_cameraSession->viewfinderSettings();
}

void QAndroidViewfinderSettingsControl2::setViewfinderSettings(const QCameraViewfinderSettings &settings)
{
    m_cameraSession->setViewfinderSettings(settings);
}

QT_END_NAMESPACE

#include "moc_qandroidviewfindersettingscontrol.cpp"

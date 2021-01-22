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

#ifndef CAMERABINCAPTURESERVICE_H
#define CAMERABINCAPTURESERVICE_H

#include <qmediaservice.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE
class QAudioInputSelectorControl;
class QVideoDeviceSelectorControl;


class CameraBinSession;
class CameraBinControl;
class QGstreamerMessage;
class QGstreamerBusHelper;
class QGstreamerVideoRenderer;
class QGstreamerVideoWindow;
class QGstreamerVideoWidgetControl;
class QGstreamerElementFactory;
class CameraBinMetaData;
class CameraBinImageCapture;
class CameraBinMetaData;
class CameraBinViewfinderSettings;
class CameraBinViewfinderSettings2;

class CameraBinService : public QMediaService
{
    Q_OBJECT

public:
    CameraBinService(GstElementFactory *sourceFactory, QObject *parent = 0);
    virtual ~CameraBinService();

    QMediaControl *requestControl(const char *name) override;
    void releaseControl(QMediaControl *) override;

    static bool isCameraBinAvailable();

private:
    void setAudioPreview(GstElement*);

    CameraBinSession *m_captureSession;
    CameraBinMetaData *m_metaDataControl;

    QAudioInputSelectorControl *m_audioInputSelector;
    QVideoDeviceSelectorControl *m_videoInputDevice;

    QMediaControl *m_videoOutput;

    QMediaControl *m_videoRenderer;
    QGstreamerVideoWindow *m_videoWindow;
#if defined(HAVE_WIDGETS)
    QGstreamerVideoWidgetControl *m_videoWidgetControl;
#endif
    CameraBinImageCapture *m_imageCaptureControl;
    QMediaControl *m_cameraInfoControl;

    CameraBinViewfinderSettings *m_viewfinderSettingsControl;
    CameraBinViewfinderSettings2 *m_viewfinderSettingsControl2;
};

QT_END_NAMESPACE

#endif // CAMERABINCAPTURESERVICE_H

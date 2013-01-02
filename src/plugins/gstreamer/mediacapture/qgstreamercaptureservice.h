/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGSTREAMERCAPTURESERVICE_H
#define QGSTREAMERCAPTURESERVICE_H

#include <qmediaservice.h>
#include <qmediacontrol.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE
class QAudioInputSelectorControl;
class QVideoDeviceSelectorControl;

class QGstreamerCaptureSession;
class QGstreamerCameraControl;
class QGstreamerMessage;
class QGstreamerBusHelper;
class QGstreamerVideoRenderer;
class QGstreamerVideoOverlay;
class QGstreamerVideoWidgetControl;
class QGstreamerElementFactory;
class QGstreamerCaptureMetaDataControl;
class QGstreamerImageCaptureControl;
class QGstreamerV4L2Input;

class QGstreamerCaptureService : public QMediaService
{
    Q_OBJECT

public:
    QGstreamerCaptureService(const QString &service, QObject *parent = 0);
    virtual ~QGstreamerCaptureService();

    QMediaControl *requestControl(const char *name);
    void releaseControl(QMediaControl *);

private:
    void setAudioPreview(GstElement *);

    QGstreamerCaptureSession *m_captureSession;
    QGstreamerCameraControl *m_cameraControl;
    QGstreamerV4L2Input *m_videoInput;
    QGstreamerCaptureMetaDataControl *m_metaDataControl;

    QAudioInputSelectorControl *m_audioInputSelector;
    QVideoDeviceSelectorControl *m_videoInputDevice;

    QMediaControl *m_videoOutput;

    QGstreamerVideoRenderer *m_videoRenderer;
#if defined(HAVE_XVIDEO) && defined(HAVE_WIDGETS)
    QMediaControl *m_videoWindow;
    QMediaControl *m_videoWidgetControl;
#endif
    QGstreamerImageCaptureControl *m_imageCaptureControl;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURESERVICE_H

/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef CAMERABINIMAGECAPTURECONTROL_H
#define CAMERABINIMAGECAPTURECONTROL_H

#include <qcameraimagecapturecontrol.h>
#include "camerabinsession.h"

QT_USE_NAMESPACE

class CameraBinImageCapture : public QCameraImageCaptureControl
{
    Q_OBJECT
public:
    CameraBinImageCapture(CameraBinSession *session);
    virtual ~CameraBinImageCapture();

    QCameraImageCapture::DriveMode driveMode() const { return QCameraImageCapture::SingleImageCapture; }
    void setDriveMode(QCameraImageCapture::DriveMode) {}

    bool isReadyForCapture() const;
    int capture(const QString &fileName);
    void cancelCapture();

private slots:
    void updateState();
    void handleBusMessage(const QGstreamerMessage &message);

private:
    static gboolean metadataEventProbe(GstPad *pad, GstEvent *event, CameraBinImageCapture *);
    static gboolean uncompressedBufferProbe(GstPad *pad, GstBuffer *buffer, CameraBinImageCapture *);
    static gboolean jpegBufferProbe(GstPad *pad, GstBuffer *buffer, CameraBinImageCapture *);
    static gboolean handleImageSaved(GstElement *camera, const gchar *filename, CameraBinImageCapture *);

    CameraBinSession *m_session;
    bool m_ready;
    int m_requestId;
    GstElement *m_jpegEncoderElement;
    GstElement *m_metadataMuxerElement;
};

#endif // CAMERABINCAPTURECORNTROL_H

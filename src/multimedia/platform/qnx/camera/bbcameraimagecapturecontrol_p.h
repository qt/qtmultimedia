/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
******************************************************************************/
#ifndef BBCAMERAIMAGECAPTURECONTROL_H
#define BBCAMERAIMAGECAPTURECONTROL_H

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

#include <private/qplatformimagecapture_p.h>

QT_BEGIN_NAMESPACE

class BbCameraSession;

class BbCameraImageCaptureControl : public QPlatformImageCapture
{
    Q_OBJECT
public:
    explicit BbCameraImageCaptureControl(BbCameraSession *session, QObject *parent = 0);

    bool isReadyForCapture() const override;

    int capture(const QString &fileName) override;
    int captureToBuffer() override;
    void cancelCapture() override;

    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

private:
    BbCameraSession *m_session;
};

QT_END_NAMESPACE

#endif

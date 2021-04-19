/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKCAMERACAPTURECONTROL_H
#define MOCKCAMERACAPTURECONTROL_H

#include <QDateTime>
#include <QTimer>
#include <QtMultimedia/qmediametadata.h>

#include "private/qplatformcameraimagecapture_p.h"
#include "private/qplatformcamera_p.h"

class QMockMediaCaptureSession;

class QMockImageCapture : public QPlatformCameraImageCapture
{
    Q_OBJECT
public:
    QMockImageCapture(QMockMediaCaptureSession *captureSession);

    ~QMockImageCapture()
    {
    }

    bool isReadyForCapture() const;

    int capture(const QString &fileName);
    int captureToBuffer() { return -1; }

    QImageEncoderSettings imageSettings() const { return m_settings; }
    void setImageSettings(const QImageEncoderSettings &settings) { m_settings = settings; }

private Q_SLOTS:
    void captured();

private:
    QMockMediaCaptureSession *m_captureSession = nullptr;
    QString m_fileName;
    int m_captureRequest = 0;
    bool m_ready = true;
    QImageEncoderSettings m_settings;
};

#endif // MOCKCAMERACAPTURECONTROL_H

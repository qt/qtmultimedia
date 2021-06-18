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

#ifndef QMOCKCAMERAIMAGECAPTURE_H
#define QMOCKCAMERAIMAGECAPTURE_H

#include <QDateTime>
#include <QTimer>
#include <QtMultimedia/qmediametadata.h>

#include "private/qplatformimagecapture_p.h"
#include "private/qplatformcamera_p.h"

QT_BEGIN_NAMESPACE

class QMockMediaCaptureSession;

class QMockImageCapture : public QPlatformImageCapture
{
    Q_OBJECT
public:
    QMockImageCapture(QImageCapture *parent);

    ~QMockImageCapture()
    {
    }

    bool isReadyForCapture() const override;

    int capture(const QString &fileName) override;
    int captureToBuffer() override { return -1; }

    QImageEncoderSettings imageSettings() const override { return m_settings; }
    void setImageSettings(const QImageEncoderSettings &settings) override { m_settings = settings; }

private Q_SLOTS:
    void captured();

private:
    QString m_fileName;
    int m_captureRequest = 0;
    bool m_ready = true;
    QImageEncoderSettings m_settings;
};

QT_END_NAMESPACE

#endif // QMOCKCAMERAIMAGECAPTURE_H

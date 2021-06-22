/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCAMERAIMAGECAPTURECONTROL_H
#define QCAMERAIMAGECAPTURECONTROL_H

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

#include <QtMultimedia/qimagecapture.h>
#include <QtMultimedia/qmediametadata.h>
#include <QtMultimedia/qimagecapture.h>

QT_BEGIN_NAMESPACE

class QImage;
class QPlatformMediaCaptureSession;

class QImageEncoderSettingsPrivate;
class Q_MULTIMEDIA_EXPORT QImageEncoderSettings
{
    QImageCapture::FileFormat m_format = QImageCapture::UnspecifiedFormat;
    QSize m_resolution;
    QImageCapture::Quality m_quality = QImageCapture::NormalQuality;

public:
    bool operator==(const QImageEncoderSettings &other) {
        return m_format == other.m_format &&
               m_resolution == other.m_resolution &&
               m_quality == other.m_quality;
    }
    bool operator!=(const QImageEncoderSettings &other) { return !operator==(other); }

    QImageCapture::FileFormat format() const { return m_format; }
    void setFormat(QImageCapture::FileFormat f) { m_format = f; }

    QSize resolution() const { return m_resolution; }
    void setResolution(const QSize &s) { m_resolution = s; }
    void setResolution(int width, int height) { m_resolution = QSize(width, height); }

    QImageCapture::Quality quality() const { return m_quality; }
    void setQuality(QImageCapture::Quality quality) { m_quality = quality; }
};

class Q_MULTIMEDIA_EXPORT QPlatformImageCapture : public QObject
{
    Q_OBJECT

public:
    virtual bool isReadyForCapture() const = 0;

    virtual int capture(const QString &fileName) = 0;
    virtual int captureToBuffer() = 0;

    virtual QImageEncoderSettings imageSettings() const = 0;
    virtual void setImageSettings(const QImageEncoderSettings &settings) = 0;

    virtual void setMetaData(const QMediaMetaData &m) { m_metaData = m; }
    QMediaMetaData metaData() const { return m_metaData; }

    QImageCapture *imageCapture() { return m_imageCapture; }

    static QString msgCameraNotReady();
    static QString msgImageCaptureNotSet();

Q_SIGNALS:
    void readyForCaptureChanged(bool ready);

    void imageExposed(int requestId);
    void imageCaptured(int requestId, const QImage &preview);
    void imageMetadataAvailable(int id, const QMediaMetaData &);
    void imageAvailable(int requestId, const QVideoFrame &buffer);
    void imageSaved(int requestId, const QString &fileName);

    void error(int id, int error, const QString &errorString);

protected:
    explicit QPlatformImageCapture(QImageCapture *parent = nullptr);
private:
    QImageCapture *m_imageCapture = nullptr;
    QMediaMetaData m_metaData;
};

QT_END_NAMESPACE


#endif  // QCAMERAIMAGECAPTURECONTROL_H


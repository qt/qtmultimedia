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

#ifndef QCAMERAIMAGECAPTURE_H
#define QCAMERAIMAGECAPTURE_H

#include <QtCore/qobject.h>
#include <QtMultimedia/qmediaencodersettings.h>
#include <QtMultimedia/qvideoframe.h>

#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE

class QSize;
class QMediaMetaData;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

class QImageEncoderSettings;
class QCamera;

class QCameraImageCapturePrivate;
class Q_MULTIMEDIA_EXPORT QCameraImageCapture : public QObject
{
    Q_OBJECT
    Q_ENUMS(Error)
    Q_ENUMS(CaptureDestination)
    Q_PROPERTY(bool readyForCapture READ isReadyForCapture NOTIFY readyForCaptureChanged)
public:
    enum Error
    {
        NoError,
        NotReadyError,
        ResourceError,
        OutOfSpaceError,
        NotSupportedFeatureError,
        FormatError
    };

    enum CaptureDestination
    {
        CaptureToFile = 0x01,
        CaptureToBuffer = 0x02
    };
    Q_DECLARE_FLAGS(CaptureDestinations, CaptureDestination)

    explicit QCameraImageCapture(QCamera *camera);
    ~QCameraImageCapture();

    bool isAvailable() const;

    QCamera *camera() const;

    Error error() const;
    QString errorString() const;

    bool isReadyForCapture() const;

    QImageEncoderSettings encodingSettings() const;
    void setEncodingSettings(const QImageEncoderSettings& settings);

    CaptureDestinations captureDestination() const;
    void setCaptureDestination(CaptureDestinations destination);

    QMediaMetaData metaData() const;
    void setMetaData(const QMediaMetaData &metaData);
    void addMetaData(const QMediaMetaData &metaData);

public Q_SLOTS:
    int capture(const QString &location = QString());
    void cancelCapture();

Q_SIGNALS:
    void error(int id, QCameraImageCapture::Error error, const QString &errorString);

    void readyForCaptureChanged(bool ready);
    void captureDestinationChanged(QCameraImageCapture::CaptureDestinations destination);

    void imageExposed(int id);
    void imageCaptured(int id, const QImage &preview);
    void imageMetadataAvailable(int id, const QMediaMetaData &metaData);
    void imageAvailable(int id, const QVideoFrame &frame);
    void imageSaved(int id, const QString &fileName);

protected:
    QCameraImageCapturePrivate *d_ptr;
private:
    Q_DISABLE_COPY(QCameraImageCapture)
    Q_DECLARE_PRIVATE(QCameraImageCapture)
    Q_PRIVATE_SLOT(d_func(), void _q_error(int, int, const QString &))
    Q_PRIVATE_SLOT(d_func(), void _q_readyChanged(bool))
    Q_PRIVATE_SLOT(d_func(), void _q_serviceDestroyed())
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QCameraImageCapture::CaptureDestinations)

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QCameraImageCapture, Error)
Q_MEDIA_ENUM_DEBUG(QCameraImageCapture, CaptureDestination)

#endif


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
#include <QtMultimedia/qvideoframe.h>

#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE

class QSize;
class QMediaMetaData;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

class QImageEncoderSettings;
class QCamera;
class QMediaCaptureSession;

class QImageCapturePrivate;
class Q_MULTIMEDIA_EXPORT QImageCapture : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool readyForCapture READ isReadyForCapture NOTIFY readyForCaptureChanged)
    Q_PROPERTY(QMediaMetaData metaData READ metaData WRITE setMetaData NOTIFY metaDataChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    Q_PROPERTY(FileFormat fileFormat READ fileFormat NOTIFY setFileFormat NOTIFY fileFormatChanged)
    Q_PROPERTY(Quality quality READ quality NOTIFY setQuality NOTIFY qualityChanged)
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
    Q_ENUM(Error)

    enum Quality
    {
        VeryLowQuality,
        LowQuality,
        NormalQuality,
        HighQuality,
        VeryHighQuality
    };
    Q_ENUM(Quality)

    enum FileFormat {
        UnspecifiedFormat,
        JPEG,
        PNG,
        WebP,
        Tiff,
        LastFileFormat = Tiff
    };
    Q_ENUM(FileFormat)

    explicit QImageCapture(QObject *parent = nullptr);
    ~QImageCapture();

    bool isAvailable() const;

    QMediaCaptureSession *captureSession() const;

    Error error() const;
    QString errorString() const;

    bool isReadyForCapture() const;

    FileFormat fileFormat() const;
    void setFileFormat(FileFormat format);

    static QList<FileFormat> supportedFormats();
    static QString fileFormatName(FileFormat c);
    static QString fileFormatDescription(FileFormat c);

    QSize resolution() const;
    void setResolution(const QSize &);
    void setResolution(int width, int height);

    Quality quality() const;
    void setQuality(Quality quality);

    QMediaMetaData metaData() const;
    void setMetaData(const QMediaMetaData &metaData);
    void addMetaData(const QMediaMetaData &metaData);

public Q_SLOTS:
    int captureToFile(const QString &location = QString());
    int capture();

Q_SIGNALS:
    void errorChanged();
    void errorOccurred(int id, QImageCapture::Error error, const QString &errorString);

    void readyForCaptureChanged(bool ready);
    void metaDataChanged();

    void fileFormatChanged();
    void qualityChanged();
    void resolutionChanged();

    void imageExposed(int id);
    void imageCaptured(int id, const QImage &preview);
    void imageMetadataAvailable(int id, const QMediaMetaData &metaData);
    void imageAvailable(int id, const QVideoFrame &frame);
    void imageSaved(int id, const QString &fileName);

private:
    // This is here to flag an incompatibilities with Qt 5
    QImageCapture(QCamera *) = delete;

    friend class QMediaCaptureSession;
    class QPlatformImageCapture *platformImageCapture();
    void setCaptureSession(QMediaCaptureSession *session);
    QImageCapturePrivate *d_ptr;
    Q_DISABLE_COPY(QImageCapture)
    Q_DECLARE_PRIVATE(QImageCapture)
    Q_PRIVATE_SLOT(d_func(), void _q_error(int, int, const QString &))
};

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QImageCapture, Error)

#endif


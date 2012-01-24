/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVECAMERACAPTURE_H
#define QDECLARATIVECAMERACAPTURE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qcamera.h>
#include <qcameraimagecapture.h>
#include <qmediaencodersettings.h>

#include <QtDeclarative/qdeclarative.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDeclarativeCamera;
class QMetaDataWriterControl;

class QDeclarativeCameraCapture : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReadyForCapture NOTIFY readyForCaptureChanged)
    Q_PROPERTY(QString capturedImagePath READ capturedImagePath NOTIFY imageSaved)
    Q_PROPERTY(QSize resolution READ resolution WRITE setResolution NOTIFY resolutionChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY captureFailed)

public:
    ~QDeclarativeCameraCapture();

    bool isReadyForCapture() const;

    QSize resolution();

    QString capturedImagePath() const;
    QCameraImageCapture::Error error() const;
    QString errorString() const;

public Q_SLOTS:
    void capture();
    void captureToLocation(const QString &location);
    void cancelCapture();

    void setResolution(const QSize &resolution);
    void setMetadata(const QString &key, const QVariant &value);

Q_SIGNALS:
    void readyForCaptureChanged(bool);

    void imageExposed();
    void imageCaptured(const QString &preview);
    void imageMetadataAvailable(const QString &key, const QVariant &value);
    void imageSaved(const QString &path);
    void captureFailed(const QString &message);

    void resolutionChanged(const QSize &);

private slots:
    void _q_imageCaptured(int, const QImage&);
    void _q_imageSaved(int, const QString&);
    void _q_imageMetadataAvailable(int, const QString &, const QVariant &);
    void _q_captureFailed(int, QCameraImageCapture::Error, const QString&);

private:
    friend class QDeclarativeCamera;
    QDeclarativeCameraCapture(QCamera *camera, QObject *parent = 0);

    QCamera *m_camera;
    QCameraImageCapture *m_capture;
    QImageEncoderSettings m_imageSettings;
    QString m_capturedImagePath;
    QMetaDataWriterControl *m_metadataWriterControl;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QT_PREPEND_NAMESPACE(QDeclarativeCameraCapture))

QT_END_HEADER

#endif

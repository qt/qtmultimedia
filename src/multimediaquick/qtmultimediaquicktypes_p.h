/****************************************************************************
**
** Copyright (C) 2021 The Qt Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QTMULTIMEDIAQUICKTYPES_H
#define QTMULTIMEDIAQUICKTYPES_H

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

#include <QtQml/qqml.h>
#include <QtMultimedia/QtMultimedia>
#include <private/qtmultimediaquickglobal_p.h>

QT_BEGIN_NAMESPACE

struct QSoundEffectForeign
{
    Q_GADGET
    QML_FOREIGN(QSoundEffect)
    QML_NAMED_ELEMENT(SoundEffect)
};

struct QMediaPlayerForeign
{
    Q_GADGET
    QML_FOREIGN(QMediaPlayer)
    QML_NAMED_ELEMENT(MediaPlayer)
};

struct QMediaCaptureSessionForeign
{
    Q_GADGET
    QML_FOREIGN(QMediaCaptureSession)
    QML_NAMED_ELEMENT(CaptureSession) // ### MediaCaptureSession?
};

struct QCameraForeign
{
    Q_GADGET
    QML_FOREIGN(QCamera)
    QML_NAMED_ELEMENT(Camera)
};

struct QCameraImageProcessingForeign
{
    Q_GADGET
    QML_FOREIGN(QCameraImageProcessing)
    QML_UNCREATABLE("CameraImageProcessing is provided by Camera")
    QML_NAMED_ELEMENT(CameraImageProcessing)
};

struct QCameraImageCaptureForeign
{
    Q_GADGET
    QML_FOREIGN(QCameraImageCapture)
    QML_NAMED_ELEMENT(ImageCapture)
};

struct QMediaEncoderForeign
{
    Q_GADGET
    QML_FOREIGN(QMediaEncoder)
    QML_NAMED_ELEMENT(MediaEncoder)
};

struct QMediaMetaDataForeign
{
    Q_GADGET
    QML_FOREIGN(QMediaMetaData)
    QML_NAMED_ELEMENT(mediaMetaData)
};

struct QMediaDevicesForeign
{
    Q_GADGET
    QML_SINGLETON
    QML_FOREIGN(QMediaDevices)
    QML_NAMED_ELEMENT(MediaDevices)
};

struct QAudioDeviceInfoForeign
{
    Q_GADGET
    QML_FOREIGN(QAudioDeviceInfo)
    QML_NAMED_ELEMENT(audioDeviceInfo)
};

struct QCameraInfoForeign
{
    Q_GADGET
    QML_FOREIGN(QCameraInfo)
    QML_NAMED_ELEMENT(cameraInfo)
};

struct QMediaEncoderSettingsForeign
{
    Q_GADGET
    QML_FOREIGN(QMediaEncoderSettings)
    QML_NAMED_ELEMENT(encoderSettings)
};

QT_END_NAMESPACE

#endif

/****************************************************************************
**
** Copyright (C) 2022 The Qt Company
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

struct QImageCaptureForeign
{
    Q_GADGET
    QML_FOREIGN(QImageCapture)
    QML_NAMED_ELEMENT(ImageCapture)
};

struct QMediaRecorderForeign
{
    Q_GADGET
    QML_FOREIGN(QMediaRecorder)
    QML_NAMED_ELEMENT(MediaRecorder)
};

struct QMediaMetaDataForeign
{
    Q_GADGET
    QML_FOREIGN(QMediaMetaData)
    QML_NAMED_ELEMENT(mediaMetaData)
};

namespace QMediaMetaDataNamespaceForeign
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(QMediaMetaData)
    QML_NAMED_ELEMENT(MediaMetaData)
};

struct QMediaDevicesForeign
{
    Q_GADGET
    QML_FOREIGN(QMediaDevices)
    QML_NAMED_ELEMENT(MediaDevices)
};

struct QAudioInputForeign
{
    Q_GADGET
    QML_FOREIGN(QAudioInput)
    QML_NAMED_ELEMENT(AudioInput)
};

struct QAudioOutputForeign
{
    Q_GADGET
    QML_FOREIGN(QAudioOutput)
    QML_NAMED_ELEMENT(AudioOutput)
};

struct QAudioDeviceForeign
{
    Q_GADGET
    QML_FOREIGN(QAudioDevice)
    QML_NAMED_ELEMENT(audioDevice)
};

namespace QAudioDeviceNamespaceForeign
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(QAudioDevice)
    QML_NAMED_ELEMENT(AudioDevice)
};

struct QCameraDeviceForeign
{
    Q_GADGET
    QML_FOREIGN(QCameraDevice)
    QML_NAMED_ELEMENT(cameraDevice)
};

namespace QCameraDeviceNamespaceForeign
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(QCameraDevice)
    QML_NAMED_ELEMENT(CameraDevice)
};

struct QMediaFormatForeign
{
    Q_GADGET
    QML_FOREIGN(QMediaFormat)
    QML_NAMED_ELEMENT(mediaFormat)
};

namespace QMediaFormatNamespaceForeign
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(QMediaFormat)
    QML_NAMED_ELEMENT(MediaFormat)
};

struct QCameraFormatForeign
{
    Q_GADGET
    QML_FOREIGN(QCameraFormat)
    QML_NAMED_ELEMENT(cameraFormat)
};

QT_END_NAMESPACE

#endif

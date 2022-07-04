// Copyright (C) 2022 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

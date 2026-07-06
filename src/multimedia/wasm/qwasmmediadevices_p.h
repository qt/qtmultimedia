// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMMEDIADEVICES_H
#define QWASMMEDIADEVICES_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformaudiodevices_p.h>

#include <private/qplatformvideodevices_p.h>
#include "qwasmjs_p.h"

#include <QtCore/private/qstdweb_p.h>
#include <qaudio.h>
#include <qaudiodevice.h>
#include <qcameradevice.h>
#include <qset.h>
#include <QtCore/qloggingcategory.h>

#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>

#include <string_view>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qWasmMediaDevices)

class QWasmAudioEngine;
class QWasmMediaDevices;

class QWasmCameraDevices : public QPlatformVideoDevices
{
    Q_OBJECT
public:
    QWasmCameraDevices(QPlatformMediaIntegration *integration);

    using QPlatformVideoDevices::onVideoInputsChanged;

protected:
    QList<QCameraDevice> findVideoInputs() const override;
    void connectNotify(const QMetaMethod &signal) override;

private:
    // weak
    QWasmMediaDevices *m_mediaDevices;
};

class QWasmAudioDevices : public QPlatformAudioDevices
{
public:
    QWasmAudioDevices();

    QPlatformAudioSource *createAudioSource(const QAudioDevice &, const QAudioFormat &,
                                            QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &, const QAudioFormat &,
                                        QObject *parent) override;

    QLatin1String backendName() const override { return QLatin1String{ "WebAssembly" }; }
    using QPlatformAudioDevices::onAudioInputsChanged;
    using QPlatformAudioDevices::onAudioOutputsChanged;

protected:
    QList<QAudioDevice> findAudioInputs() const override;
    QList<QAudioDevice> findAudioOutputs() const override;
    void connectNotify(const QMetaMethod &signal) override;

};

class QWasmMediaDevices
{
public:
    QWasmMediaDevices();
    static QWasmMediaDevices *instance();
    QList<QCameraDevice> videoInputs() const;

    QList<QAudioDevice> audioInputs() const;
    QList<QAudioDevice> audioOutputs() const;

    void initDevices();
    void getMediaDevices();

private:
    void updateCameraDevices();
    void parseDevices(emscripten::val devices);
    void insertFallbackAudioOutput();
    QByteArray findDefaultDeviceId(emscripten::val devices, std::string_view deviceKind) const;
    static void applyDefaultDevice(QList<QAudioDevice> &audioDevices,
                                   const QByteArray &defaultDeviceId);
    static void applyDefaultDevice(QList<QCameraDevice> &cameraDevices,
                                   const QByteArray &defaultDeviceId);

    // insertion order, stable across hot-plug
    QList<QAudioDevice> m_audioOutputs;
    QList<QAudioDevice> m_audioInputs;
    QList<QCameraDevice> m_cameraDevices;

    std::unique_ptr<qstdweb::EventCallback> m_deviceChangedCallback;

    bool m_videoInputsAdded = false;
    bool m_audioInputsAdded = false;
    bool m_audioOutputsAdded = false;
    emscripten::val m_jsMediaDevicesInterface = emscripten::val::undefined();
    bool m_initDone = false;
    emscripten::val devicesList;
};

QT_END_NAMESPACE

#endif

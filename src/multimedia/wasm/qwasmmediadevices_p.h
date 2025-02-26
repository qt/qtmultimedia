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

#include <QtCore/private/qstdweb_p.h>
#include <qaudio.h>
#include <qaudiodevice.h>
#include <qcameradevice.h>
#include <qset.h>
#include <QtCore/qloggingcategory.h>

#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <QMapIterator>
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

private:
    // weak
    QWasmMediaDevices *m_mediaDevices;
};

// TODO: get rid of the inheritance. Instead, we should create QWasmAudioDevices,
// and use QWasmMediaDevices in both, QWasmAudioDevices and QWasmVideoDevices.
class QWasmMediaDevices : public QPlatformAudioDevices
{
public:
    QWasmMediaDevices();

    QList<QCameraDevice> videoInputs() const;

    QPlatformAudioSource *createAudioSource(const QAudioDevice &, const QAudioFormat &,
                                            QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &, const QAudioFormat &,
                                        QObject *parent) override;
    void initDevices();

    QLatin1String backendName() const override { return QLatin1String{ "WebAssembly" }; }

protected:
    QList<QAudioDevice> findAudioInputs() const override;
    QList<QAudioDevice> findAudioOutputs() const override;

private:
    void updateCameraDevices();
    void getMediaDevices();
    void getOpenALAudioDevices();
    void parseDevices(emscripten::val devices);

    QMap <std::string, QAudioDevice> m_audioOutputs;
    QMap <std::string, QAudioDevice> m_audioInputs;
    QMap <std::string, QCameraDevice> m_cameraDevices;


    std::unique_ptr<qstdweb::EventCallback> m_deviceChangedCallback;

    bool m_videoInputsAdded = false;
    bool m_audioInputsAdded = false;
    bool m_audioOutputsAdded = false;
    emscripten::val m_jsMediaDevicesInterface = emscripten::val::undefined();
    bool m_initDone = false;
    bool m_firstInit = false;
};

QT_END_NAMESPACE

#endif

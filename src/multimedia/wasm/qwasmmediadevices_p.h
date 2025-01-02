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

#include <private/qplatformmediadevices_p.h>

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

class QWasmCameraDevices : public QPlatformVideoDevices
{
    Q_OBJECT
public:
    QWasmCameraDevices(QPlatformMediaIntegration *integration);

    QList<QCameraDevice> videoDevices() const override;
private:
    // weak
    QPlatformMediaDevices *m_mediaDevices;
};

class QWasmMediaDevices : public QPlatformMediaDevices
{
public:
    QWasmMediaDevices();

    QList<QAudioDevice> audioInputs() const override;
    QList<QAudioDevice> audioOutputs() const override;
    QList<QCameraDevice> videoInputs() const;

    QPlatformAudioSource *createAudioSource(const QAudioDevice &deviceInfo,
                                            QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &deviceInfo,
                                        QObject *parent) override;
    void initDevices();

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

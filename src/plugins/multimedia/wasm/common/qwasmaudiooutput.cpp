// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR
// GPL-2.0-only OR GPL-3.0-only

#include <qaudiodevice.h>
#include <qaudiooutput.h>
#include <qwasmaudiooutput_p.h>
//#include <qwasmaudiodevice_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qWasmMediaAudioOutput, "qt.multimedia.wasm.audiooutput")

QWasmAudioOutput::QWasmAudioOutput(QAudioOutput *parent)
    : QPlatformAudioOutput(parent)
{
    // TODO
}

QWasmAudioOutput::~QWasmAudioOutput() { }

void QWasmAudioOutput::setAudioDevice(const QAudioDevice &device) {
    Q_UNUSED(device);
}

void QWasmAudioOutput::setMuted(bool muted) {
    Q_UNUSED(muted);
}

void QWasmAudioOutput::setVolume(float volume) {
    Q_UNUSED(volume);
}

QT_END_NAMESPACE

// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMAUDIOINPUT_H
#define QWASMAUDIOINPUT_H

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

#include <QtCore/qobject.h>
#include <QtCore/qloggingcategory.h>

#include <private/qtmultimediaglobal_p.h>
#include <private/qplatformaudioinput_p.h>

#include <emscripten.h>
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qWasmAudioInput)

class QWasmAudioInput : public QObject, public QPlatformAudioInput
{
    Q_OBJECT
public:
    explicit QWasmAudioInput(QAudioInput *parent);
    ~QWasmAudioInput();

    void setMuted(bool muted) override;
    void setAudioDevice(const QAudioDevice & device) final;

    bool isMuted() const;
    void setVolume(float volume) final;
    emscripten::val mediaStream();


Q_SIGNALS:
    void mutedChanged(bool muted);

private:
    bool m_wasMuted = false;
    void setDeviceSourceStream(const std::string &id);
    emscripten::val m_mediaStream;
};

QT_END_NAMESPACE

#endif // QWASMAUDIOINPUT_H

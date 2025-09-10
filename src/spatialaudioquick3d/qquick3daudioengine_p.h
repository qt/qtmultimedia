// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#ifndef QQUICK3DAUDIOENGINE_H
#define QQUICK3DAUDIOENGINE_H

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

#include <private/qquick3dnode_p.h>
#include <QtGui/qvector3d.h>
#include <qaudioengine.h>

QT_BEGIN_NAMESPACE

class QQuick3DSpatialSound;

class QQuick3DAudioEngine : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AudioEngine)
    Q_PROPERTY(OutputMode outputMode READ outputMode WRITE setOutputMode NOTIFY outputModeChanged)
    Q_PROPERTY(QAudioDevice outputDevice READ outputDevice WRITE setOutputDevice NOTIFY outputDeviceChanged)
    Q_PROPERTY(float masterVolume READ masterVolume WRITE setMasterVolume NOTIFY masterVolumeChanged)

public:
    // Keep in sync with QAudioEngine::OutputMode
    enum OutputMode {
        Surround,
        Stereo,
        Headphone
    };
    Q_ENUM(OutputMode)

    QQuick3DAudioEngine();
    ~QQuick3DAudioEngine() override;

    void setOutputMode(OutputMode mode);
    OutputMode outputMode() const;

    void setOutputDevice(const QAudioDevice &device);
    QAudioDevice outputDevice() const;

    void setMasterVolume(float volume);
    float masterVolume() const;

    static QAudioEngine *getEngine();

Q_SIGNALS:
    void outputModeChanged();
    void outputDeviceChanged();
    void masterVolumeChanged();
};

QT_END_NAMESPACE

#endif

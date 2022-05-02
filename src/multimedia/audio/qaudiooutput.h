/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QAUDIOOUTPUTDEVICE_H
#define QAUDIOOUTPUTDEVICE_H

#include <QtCore/qobject.h>
#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qaudio.h>

#include <functional>

QT_BEGIN_NAMESPACE

class QAudioDevice;
class QPlatformAudioOutput;

class Q_MULTIMEDIA_EXPORT QAudioOutput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAudioDevice device READ device WRITE setDevice NOTIFY deviceChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)

public:
    explicit QAudioOutput(QObject *parent = nullptr);
    explicit QAudioOutput(const QAudioDevice &device, QObject *parent = nullptr);
    ~QAudioOutput();

    QAudioDevice device() const;
    float volume() const;
    bool isMuted() const;

public Q_SLOTS:
    void setDevice(const QAudioDevice &device);
    void setVolume(float volume);
    void setMuted(bool muted);

Q_SIGNALS:
    void deviceChanged();
    void volumeChanged(float volume);
    void mutedChanged(bool muted);

private:
    QPlatformAudioOutput *handle() const { return d; }
    void setDisconnectFunction(std::function<void()> disconnectFunction);
    friend class QMediaCaptureSession;
    friend class QMediaPlayer;
    Q_DISABLE_COPY(QAudioOutput)
    QPlatformAudioOutput *d = nullptr;
};

QT_END_NAMESPACE

#endif  // QAUDIOOUTPUTDEVICE_H

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDAUDIODEVICE_H
#define QANDROIDAUDIODEVICE_H

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

#include <private/qaudiodevice_p.h>

QT_BEGIN_NAMESPACE

class QAndroidAudioDevice : public QAudioDevicePrivate
{
public:
    QAndroidAudioDevice(QByteArray device,
                        QString desc,
                        QAudioDevice::Mode mode,
                        QAudioFormat preferredFormat,
                        bool isBluetoothDevice,
                        bool isDefaultDevice = false);
    ~QAndroidAudioDevice() {}

    bool isBluetoothDevice() const;

private:
    bool m_isBluetoothDevice;
};

QT_END_NAMESPACE

#endif // QANDROIDAUDIODEVICE_H

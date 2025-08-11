// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDAUDIOUTIL_H
#define QANDROIDAUDIOUTIL_H

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

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qtmultimediaexports.h>

QT_BEGIN_NAMESPACE

namespace QAndroidAudioUtil {

Q_MULTIMEDIA_EXPORT bool supportsLowLatency();
Q_MULTIMEDIA_EXPORT bool isDefaultBluetoothDevice(const QAudioDevice &device);

} // namespace QAndroidAudioUtil

QT_END_NAMESPACE

#endif // QANDROIDAUDIOUTIL_H

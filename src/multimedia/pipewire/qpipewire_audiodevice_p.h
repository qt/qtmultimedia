// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIODEVICE_P_H
#define QPIPEWIRE_AUDIODEVICE_P_H

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

#include <QtCore/qlist.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qaudiodevice_p.h>

#include "qpipewire_propertydict_p.h"
#include "qpipewire_spa_pod_support_p.h"

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class Q_MULTIMEDIA_EXPORT QPipewireAudioDevicePrivate : public QAudioDevicePrivate
{
public:
    QPipewireAudioDevicePrivate(const PwPropertyDict &nodeProperties,
                                const PwPropertyDict &deviceProperties,
                                const SpaObjectAudioFormat &, QAudioDevice::Mode, bool isDefault);
    ~QPipewireAudioDevicePrivate() override;

    Q_DISABLE_COPY_MOVE(QPipewireAudioDevicePrivate)

    QByteArray nodeName() const { return m_nodeName; }

private:
    void setPreferredSamplingRate(int);
    void setPreferredSamplingRate(QSpan<const int>);
    void setPreferredSamplingRate(const SpaRange<int> &);

    void setPreferredSampleFormats(spa_audio_format);
    void setPreferredSampleFormats(const SpaEnum<spa_audio_format> &);

    QByteArray m_sysfsPath;
    QByteArray m_nodeName;

    QList<spa_audio_channel> m_channelPositions;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_AUDIODEVICE_P_H

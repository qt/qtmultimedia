// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOREAUDIOUTILS_P_H
#define QCOREAUDIOUTILS_P_H

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

#include <AudioUnit/AudioUnit.h>
#include <QtCore/qglobal.h>
#include <QtCore/qpromise.h>
#include <QtCore/qfuture.h>
#include <QtCore/qspan.h>
#include <QtCore/private/quniquehandle_p.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qaudiodevice.h>

#include <CoreAudioTypes/CoreAudioTypes.h>
#ifdef Q_OS_MACOS
#  include <CoreAudio/AudioHardwareBase.h>
#endif

QT_BEGIN_NAMESPACE

namespace QCoreAudioUtils {

struct QFreeDeleter
{
    template <typename T>
    void operator()(T *t)
    {
        ::free(t);
    }
};

Q_MULTIMEDIA_EXPORT QAudioFormat toQAudioFormat(const AudioStreamBasicDescription &streamFormat);
AudioStreamBasicDescription toAudioStreamBasicDescription(QAudioFormat const &audioFormat);

Q_MULTIMEDIA_EXPORT std::unique_ptr<AudioChannelLayout, QFreeDeleter>
toAudioChannelLayout(const QAudioFormat &format, UInt32 *size);
QAudioFormat::ChannelConfig fromAudioChannelLayout(const AudioChannelLayout *layout);

#ifdef Q_OS_MACOS
class DeviceDisconnectMonitor
{
public:
    bool addDisconnectListener(AudioObjectID);
    void removeDisconnectListener();

    template <typename Functor>
    auto then(QObject *context, Functor &&f)
    {
        return m_disconnectedFuture.then(context, std::forward<Functor>(f));
    }

private:
    static OSStatus disconnectCallback(AudioObjectID, UInt32 numberOfAddresses,
                                       const AudioObjectPropertyAddress *inAddresses, void *self);
    OSStatus streamDisconnectListener(AudioObjectID,
                                      QSpan<const AudioObjectPropertyAddress> properties);

    QPromise<void> m_disconnectedPromise;
    QFuture<void> m_disconnectedFuture;

    AudioObjectID m_currentId;
};
#endif

struct AudioUnitHandleTraits
{
    using Type = AudioUnit;
    static Type invalidValue() { return nullptr; }
    static bool close(Type handle) { return AudioComponentInstanceDispose(handle) == noErr; }
};

struct AudioUnitHandle : QUniqueHandle<AudioUnitHandleTraits>
{
    using QUniqueHandle::QUniqueHandle;
    AudioUnitHandle(AudioUnitHandle &&) = default;
    AudioUnitHandle &operator=(AudioUnitHandle &&) = default;
    ~AudioUnitHandle();

    bool initialize();
    void deinitialize();
    bool isInitialized() const { return m_initialized; }

private:
    bool m_initialized{};
};

// AudioUnit helpers
std::optional<AudioUnitHandle> makeAudioUnitForIO();
bool audioUnitSetInputEnabled(AudioUnitHandle &, bool);
bool audioUnitSetOutputEnabled(AudioUnitHandle &, bool);
bool audioUnitSetInputStreamFormat(AudioUnitHandle &, AudioUnitElement,
                                   const AudioStreamBasicDescription &);
bool audioUnitSetOutputStreamFormat(AudioUnitHandle &, AudioUnitElement,
                                    const AudioStreamBasicDescription &);
bool audioUnitIsRunning(AudioUnitHandle &);
bool audioUnitSetRenderCallback(AudioUnitHandle &, AURenderCallbackStruct &);

std::optional<AudioStreamBasicDescription> audioUnitGetInputStreamFormat(AudioUnitHandle &,
                                                                         AudioUnitElement);

std::optional<int> audioUnitGetFramesPerSlice(AudioUnitHandle &);

#if defined(Q_OS_MACOS)
bool audioUnitSetCurrentDevice(AudioUnitHandle &, AudioObjectID);
std::optional<int> audioUnitGetFramesPerBuffer(AudioUnitHandle &);
bool audioObjectSetSamplingRate(AudioObjectID, int);
std::optional<int> audioObjectFindBestNominalSampleRate(AudioObjectID, QAudioDevice::Mode,
                                                        int rate);
bool audioObjectSetFramesPerBuffer(AudioObjectID, int32_t bufferFrames);
#endif

} // namespace QCoreAudioUtils

QT_END_NAMESPACE

#endif // QCOREAUDIOUTILS_P_H

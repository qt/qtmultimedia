// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPULSEAUDIO_CONTEXTMANAGER_P_H
#define QPULSEAUDIO_CONTEXTMANAGER_P_H

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

#include <QtCore/qmap.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qreadwritelock.h>
#include <pulse/pulseaudio.h>
#include "qpulsehelpers_p.h"
#include <qaudioformat.h>

QT_BEGIN_NAMESPACE

class QPulseAudioContextManager : public QObject
{
    Q_OBJECT

    using PAContextHandle = QPulseAudioInternal::PAContextHandle;
    using PAOperationHandle = QPulseAudioInternal::PAOperationHandle;

public:
    explicit QPulseAudioContextManager(QObject *parent = nullptr);
    ~QPulseAudioContextManager() override;

    static QPulseAudioContextManager *instance();
    pa_threaded_mainloop *mainloop() { return m_mainLoop.get(); }
    pa_context *context() { return m_context.get(); }

    void lock()
    {
        if (m_mainLoop)
            pa_threaded_mainloop_lock(m_mainLoop.get());
    }

    void unlock()
    {
        if (m_mainLoop)
            pa_threaded_mainloop_unlock(m_mainLoop.get());
    }

    void wait(const PAOperationHandle &op)
    {
        while (m_mainLoop && pa_operation_get_state(op.get()) == PA_OPERATION_RUNNING)
            pa_threaded_mainloop_wait(m_mainLoop.get());
    }

    bool waitForAsyncOperation(pa_operation *op);

    bool isInMainLoop() const
    {
        return m_mainLoop && pa_threaded_mainloop_in_thread(m_mainLoop.get());
    }

    QList<QAudioDevice> availableDevices(QAudioDevice::Mode mode) const;
    QByteArray defaultDevice(QAudioDevice::Mode mode) const;

    pa_context_state_t getContextState();
    bool contextIsGood();

    QString serverName();

Q_SIGNALS:
    void audioInputsChanged();
    void audioOutputsChanged();

private Q_SLOTS:
    void prepare();
    void onContextFailed();

private:
    static void serverInfoCallback(pa_context *context, const pa_server_info *info, void *userdata);
    static void sinkInfoCallback(pa_context *context, const pa_sink_info *info, int isLast,
                                 void *userdata);
    static void sourceInfoCallback(pa_context *context, const pa_source_info *info, int isLast,
                                   void *userdata);
    static void eventCallback(pa_context *context, pa_subscription_event_type_t t, uint32_t index,
                              void *userdata);
    static void contextStateCallbackInit(pa_context *context, void *userdata);
    static void contextStateCallback(pa_context *c, void *userdata);

    void updateDevices();
    void release();

    QMap<int, QAudioDevice> m_sinks;
    QMap<int, QAudioDevice> m_sources;

    QByteArray m_defaultSink;
    QByteArray m_defaultSource;

    mutable QReadWriteLock m_sinkLock;
    mutable QReadWriteLock m_sourceLock;
    mutable QReadWriteLock m_serverLock;

    pa_mainloop_api *m_mainLoopApi{};
    std::unique_ptr<pa_threaded_mainloop, QPulseAudioInternal::PaMainLoopDeleter> m_mainLoop;
    PAContextHandle m_context;
    bool m_prepared{};

    QString m_serverName;
 };

QT_END_NAMESPACE

#endif // QPULSEAUDIO_CONTEXTMANAGER_P_H

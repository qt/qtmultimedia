/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QPULSEAUDIOENGINE_H
#define QPULSEAUDIOENGINE_H

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
#include <QtMultimedia/qaudiosystemplugin.h>
#include <pulse/pulseaudio.h>
#include "qpulsehelpers.h"
#include <qaudioformat.h>

QT_BEGIN_NAMESPACE

class QPulseAudioEngine : public QObject
{
    Q_OBJECT

public:
    QPulseAudioEngine(QObject *parent = 0);
    ~QPulseAudioEngine();

    static QPulseAudioEngine *instance();
    pa_threaded_mainloop *mainloop() { return m_mainLoop; }
    pa_context *context() { return m_context; }

    inline void lock()
    {
        if (m_mainLoop)
            pa_threaded_mainloop_lock(m_mainLoop);
    }

    inline void unlock()
    {
        if (m_mainLoop)
            pa_threaded_mainloop_unlock(m_mainLoop);
    }

    inline void wait(pa_operation *op)
    {
        while (m_mainLoop && pa_operation_get_state(op) == PA_OPERATION_RUNNING)
            pa_threaded_mainloop_wait(m_mainLoop);
    }

    QList<QByteArray> availableDevices(QAudio::Mode mode) const;
    QByteArray defaultDevice(QAudio::Mode mode) const;

Q_SIGNALS:
    void contextFailed();

private Q_SLOTS:
    void prepare();
    void onContextFailed();

private:
    void updateDevices();
    void release();

public:
    QMap<int, QByteArray> m_sinks;
    QMap<int, QByteArray> m_sources;
    QMap<QByteArray, QAudioFormat> m_preferredFormats;

    QByteArray m_defaultSink;
    QByteArray m_defaultSource;

    mutable QReadWriteLock m_sinkLock;
    mutable QReadWriteLock m_sourceLock;
    mutable QReadWriteLock m_serverLock;

private:
    pa_mainloop_api *m_mainLoopApi;
    pa_threaded_mainloop *m_mainLoop;
    pa_context *m_context;
    bool m_prepared;
 };

QT_END_NAMESPACE

#endif

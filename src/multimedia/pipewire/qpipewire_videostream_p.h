// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_VIDEOSTREAM_P_H
#define QPIPEWIRE_VIDEOSTREAM_P_H

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

#include <QtMultimedia/private/qpipewire_cameracontextmanager_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qsize.h>

#include <spa/param/video/format.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class Q_MULTIMEDIA_EXPORT QPipeWireVideoStream : public QObject
{
    Q_OBJECT

public:
    QPipeWireVideoStream(QCameraContextManager &context, QByteArray targetObject,
                         QVideoFrameFormat::PixelFormat requestedPixelFormat, QSize requestedSize,
                         qreal requestedFrameRate);
    ~QPipeWireVideoStream() override;

    Q_DISABLE_COPY_MOVE(QPipeWireVideoStream)

    bool start();

    QVideoFrameFormat frameFormat() const;

Q_SIGNALS:
    void newVideoFrame(const QVideoFrame &frame);
    void errorOccurred(const QString &errorString);

private:
    void onStateChanged(pw_stream_state old, pw_stream_state state, const char *error);
    void onParamChanged(uint32_t id, const spa_pod *param);
    void onProcess();

    static const pw_stream_events &streamEvents();

    QCameraContextManager &m_context;
    const QByteArray m_targetObject;
    const QVideoFrameFormat::PixelFormat m_requestedPixelFormat;
    const QSize m_requestedSize;
    const qreal m_requestedFrameRate;

    PwStreamHandle m_stream;
    spa_hook m_streamListener{};

    mutable QMutex m_formatMutex;
    QVideoFrameFormat m_frameFormat;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_VIDEOSTREAM_P_H

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSVIDEOOUTPUT_P_H
#define QOHOSVIDEOOUTPUT_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qsize.h>

#include <native_image/native_image.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QVideoFrame;
class QVideoSink;
class QOhosSurfaceImage;
class QOhosTextureThread;

class QOhosVideoOutput : public QObject
{
    Q_OBJECT
public:
    // Only camera frames follow the device and need the display rotation.
    enum class ContentSource { Camera, MediaPlayer };

    explicit QOhosVideoOutput(QVideoSink *sink, ContentSource contentSource,
                              QObject *parent = nullptr);
    ~QOhosVideoOutput() override;

    OHNativeWindow *nativeWindow();
    QByteArray surfaceId();

    void setVideoSize(const QSize &size);

    QVideoSink *sink() const { return m_sink; }

signals:
    void surfaceReady();

private slots:
    void onNewFrame(const QVideoFrame &frame);
    void onRhiChanged();
    void updateDisplayRotation();

private:
    QPointer<QVideoSink> m_sink;
    ContentSource m_contentSource;
    QSize m_videoSize;
    std::shared_ptr<QOhosTextureThread> m_textureThread;
    bool m_surfaceCreatedWithoutRhi{ false };
};

QT_END_NAMESPACE

#endif // QOHOSVIDEOOUTPUT_P_H

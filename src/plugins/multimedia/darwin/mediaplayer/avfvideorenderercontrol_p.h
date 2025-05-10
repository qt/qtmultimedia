// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFVIDEORENDERERCONTROL_H
#define AVFVIDEORENDERERCONTROL_H

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

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/qsize.h>
#include <QtCore/private/qcore_mac_p.h>

#include <avfvideosink_p.h>

#include <CoreVideo/CVBase.h>
#include <CoreVideo/CVPixelBuffer.h>

Q_FORWARD_DECLARE_OBJC_CLASS(CALayer);
Q_FORWARD_DECLARE_OBJC_CLASS(AVPlayerItemVideoOutput);
Q_FORWARD_DECLARE_OBJC_CLASS(AVPlayerItemLegibleOutput);
Q_FORWARD_DECLARE_OBJC_CLASS(SubtitleDelegate);

QT_BEGIN_NAMESPACE

class AVFDisplayLink;

class AVFVideoRendererControl : public QObject, public AVFVideoSinkInterface
{
    Q_OBJECT
public:
    explicit AVFVideoRendererControl(QObject *parent = nullptr);
    ~AVFVideoRendererControl() override;

    // AVFVideoSinkInterface
    void reconfigure() override;
    void setLayer(CALayer *layer) override;

    void setVideoRotation(QtVideo::Rotation);
    void setVideoMirrored(bool mirrored);

    void setSubtitleText(const QString &subtitle)
    {
        m_sink->setSubtitleText(subtitle);
    }
private Q_SLOTS:
    void updateVideoFrame(const CVTimeStamp &ts);

private:
    AVPlayerLayer *playerLayer() const { return static_cast<AVPlayerLayer *>(m_layer); }
    QCFType<CVPixelBufferRef> copyPixelBufferFromLayer(size_t &width, size_t &height);

    QMutex m_mutex;
    AVFDisplayLink *m_displayLink = nullptr;
    AVPlayerItemVideoOutput *m_videoOutput = nullptr;
    AVPlayerItemLegibleOutput *m_subtitleOutput = nullptr;
    SubtitleDelegate *m_subtitleDelegate = nullptr;
    QtVideo::Rotation m_rotation = QtVideo::Rotation::None;
    bool m_mirrored = false;
};

QT_END_NAMESPACE

#endif // AVFVIDEORENDERERCONTROL_H

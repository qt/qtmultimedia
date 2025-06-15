// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERVIDEOOUTPUT_P_H
#define QGSTREAMERVIDEOOUTPUT_P_H

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
#include <QtCore/qpointer.h>
#include <QtCore/private/qexpected_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>
#include <common/qgst_p.h>
#include <common/qgstreamervideosink_p.h>
#include <common/qgstsubtitlesink_p.h>

QT_BEGIN_NAMESPACE

class QVideoSink;

class QGstreamerVideoOutput : public QObject, QAbstractSubtitleObserver
{
    Q_OBJECT

public:
    static q23::expected<QGstreamerVideoOutput *, QString> create(QObject *parent = nullptr);
    ~QGstreamerVideoOutput() override;

    void setVideoSink(QVideoSink *sink);
    QGstreamerVideoSink *gstreamerVideoSink() const { return m_platformVideoSink; }

    QGstElement gstElement() const { return m_outputBin; }
    QGstElement gstSubtitleElement() const { return m_subtitleSink; }

    void setActive(bool);

    void setIsPreview();
    void flushSubtitles();

    void setNativeSize(QSize);
    void setRotation(QtVideo::Rotation);

    void updateSubtitle(QString) override;

signals:
    void subtitleChanged(QString);

private:
    explicit QGstreamerVideoOutput(QObject *parent);

    void updateNativeSize();

    QPointer<QGstreamerVideoSink> m_platformVideoSink;

    // Gst elements
    QGstBin m_outputBin;
    QGstElement m_videoQueue;
    QGstElement m_videoConvertScale;
    QGstElement m_videoSink;

    QGstElement m_subtitleSink;
    QMetaObject::Connection m_subtitleConnection;
    QString m_lastSubtitleString;

    bool m_isActive{ false };
    QSize m_nativeSize;
    QtVideo::Rotation m_rotation{};
};

QT_END_NAMESPACE

#endif

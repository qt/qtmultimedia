// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGST_DISCOVERER_P_H
#define QGST_DISCOVERER_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qlocale.h>
#include <QtCore/qsize.h>
#include <QtCore/private/qexpected_p.h>

#include <QtMultimedia/private/qmultimediautils_p.h>

#include "qgst_p.h"
#include "qgst_handle_types_p.h"

#include <gst/pbutils/gstdiscoverer.h>

#include <vector>

QT_BEGIN_NAMESPACE

class QMediaMetaData;

namespace QGst {

using QGstDiscovererHandle = QGstImpl::QGstHandleHelper<GstDiscoverer>::SharedHandle;
using QGstDiscovererInfoHandle = QGstImpl::QGstHandleHelper<GstDiscovererInfo>::SharedHandle;
using QGstDiscovererAudioInfoHandle =
        QGstImpl::QGstHandleHelper<GstDiscovererAudioInfo>::SharedHandle;
using QGstDiscovererVideoInfoHandle =
        QGstImpl::QGstHandleHelper<GstDiscovererVideoInfo>::SharedHandle;
using QGstDiscovererSubtitleInfoHandle =
        QGstImpl::QGstHandleHelper<GstDiscovererSubtitleInfo>::SharedHandle;
using QGstDiscovererStreamInfoHandle =
        QGstImpl::QGstHandleHelper<GstDiscovererStreamInfo>::SharedHandle;

struct QGstDiscovererStreamInfo
{
    int streamNumber{};
    QString streamID;
    QGstTagListHandle tags;
    QGstCaps caps;
};

struct QGstDiscovererVideoInfo : QGstDiscovererStreamInfo
{
    QSize size;
    int bitDepth{};
    Fraction framerate{};
    Fraction pixelAspectRatio{};
    bool isInterlaced{};
    int bitrate{};
    int maxBitrate{};
    bool isImage{};
};

struct QGstDiscovererAudioInfo : QGstDiscovererStreamInfo
{
    int channels{};
    uint64_t channelMask{};
    int sampleRate{};
    int bitsPerSample{};
    int bitrate{};
    int maxBitrate{};
    QLocale::Language language{};
};

struct QGstDiscovererSubtitleInfo : QGstDiscovererStreamInfo
{
    QLocale::Language language{};
};

struct QGstDiscovererContainerInfo : QGstDiscovererStreamInfo
{
    QGstTagListHandle tags;
};

struct QGstDiscovererInfo
{
    bool isLive{};
    bool isSeekable{};
    std::optional<std::chrono::nanoseconds> duration;

    std::optional<QGstDiscovererContainerInfo> containerInfo;
    QGstTagListHandle tags;
    std::vector<QGstDiscovererVideoInfo> videoStreams;
    std::vector<QGstDiscovererAudioInfo> audioStreams;
    std::vector<QGstDiscovererSubtitleInfo> subtitleStreams;
    std::vector<QGstDiscovererContainerInfo> containerStreams;
};

// For now we only perform synchronous discovery. Our future selves may want to perform the
// discovery asynchronously.
class QGstDiscoverer
{
public:
    QGstDiscoverer();

    q23::expected<QGstDiscovererInfo, QUniqueGErrorHandle> discover(const QString &uri);
    q23::expected<QGstDiscovererInfo, QUniqueGErrorHandle> discover(const QUrl &);
    q23::expected<QGstDiscovererInfo, QUniqueGErrorHandle> discover(QIODevice *);

private:
    q23::expected<QGstDiscovererInfo, QUniqueGErrorHandle> discover(const char *);
    QGstDiscovererHandle m_instance;
};

QMediaMetaData toContainerMetadata(const QGstDiscovererInfo &);
QMediaMetaData toStreamMetadata(const QGstDiscovererVideoInfo &);
QMediaMetaData toStreamMetadata(const QGstDiscovererAudioInfo &);
QMediaMetaData toStreamMetadata(const QGstDiscovererSubtitleInfo &);

} // namespace QGst

QT_END_NAMESPACE

#endif // QGST_DISCOVERER_P_H

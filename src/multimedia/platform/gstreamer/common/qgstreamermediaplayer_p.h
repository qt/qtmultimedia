/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGSTREAMERMEDIAPLAYER_P_H
#define QGSTREAMERMEDIAPLAYER_P_H

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

#include <QtCore/qstack.h>
#include <private/qplatformmediaplayer_p.h>
#include <private/qtmultimediaglobal_p.h>
#include <qurl.h>
#include <private/qgst_p.h>
#include <private/qgstpipeline_p.h>

#include <QtCore/qtimer.h>

#include <array>

QT_BEGIN_NAMESPACE

class QNetworkAccessManager;
class QGstreamerMessage;
class QGstAppSrc;
class QGstreamerAudioOutput;
class QGstreamerVideoOutput;

class Q_MULTIMEDIA_EXPORT QGstreamerMediaPlayer
    : public QObject,
      public QPlatformMediaPlayer,
      public QGstreamerBusMessageFilter,
      public QGstreamerSyncMessageFilter
{
    Q_OBJECT

public:
    QGstreamerMediaPlayer(QMediaPlayer *parent = 0);
    ~QGstreamerMediaPlayer();

    qint64 position() const override;
    qint64 duration() const override;

    float bufferProgress() const override;

    QMediaTimeRange availablePlaybackRanges() const override;

    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;

    QUrl media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QUrl&, QIODevice *) override;

    bool streamPlaybackSupported() const override { return true; }

    void setAudioOutput(QPlatformAudioOutput *output) override;

    QMediaMetaData metaData() const override;

    void setVideoSink(QVideoSink *sink) override;

    int trackCount(TrackType) override;
    QMediaMetaData trackMetaData(TrackType /*type*/, int /*streamNumber*/) override;
    int activeTrack(TrackType) override;
    void setActiveTrack(TrackType, int /*streamNumber*/) override;

    void setPosition(qint64 pos) override;

    void play() override;
    void pause() override;
    void stop() override;

    bool processBusMessage(const QGstreamerMessage& message) override;
    bool processSyncMessage(const QGstreamerMessage& message) override;

    void *nativePipeline() override;

public Q_SLOTS:
    void updatePosition() { positionChanged(position()); }

private:
    struct TrackSelector {
        TrackSelector(TrackType, const char *name);
        QGstPad createInputPad();
        void removeInputPad(QGstPad pad);
        void removeAllInputPads();
        QGstPad inputPad(int index);
        int activeInputIndex() const { return isConnected ? tracks.indexOf(activeInputPad()) : -1; }
        QGstPad activeInputPad() const { return isConnected ? selector.getObject("active-pad") : QGstPad{}; }
        void setActiveInputPad(QGstPad input) { selector.set("active-pad", input); }
        int trackCount() const { return tracks.count(); }

        QGstElement selector;
        TrackType type;
        QList<QGstPad> tracks;
        bool isConnected = false;
    };

    friend class QGstreamerStreamsControl;
    void decoderPadAdded(const QGstElement &src, const QGstPad &pad);
    void decoderPadRemoved(const QGstElement &src, const QGstPad &pad);
    static void uridecodebinElementAddedCallback(GstElement *uridecodebin, GstElement *child, QGstreamerMediaPlayer *that);
    static void sourceSetupCallback(GstElement *uridecodebin, GstElement *source, QGstreamerMediaPlayer *that);
    void parseStreamsAndMetadata();
    void connectOutput(TrackSelector &ts);
    void removeOutput(TrackSelector &ts);
    void removeAllOutputs();
    void stopOrEOS(bool eos);

    std::array<TrackSelector, NTrackTypes> trackSelectors;
    TrackSelector &trackSelector(TrackType type);

    QMediaMetaData m_metaData;

    int m_bufferProgress = -1;
    QUrl m_url;
    QIODevice *m_stream = nullptr;

    bool prerolling = false;
    bool m_requiresSeekOnPlay = false;
    qint64 m_duration = 0;
    QTimer positionUpdateTimer;

    QGstAppSrc *m_appSrc = nullptr;

    GType decodebinType;
    QGstStructure topology;

    // Gst elements
    QGstPipeline playerPipeline;
    QGstElement src;
    QGstElement decoder;

    QGstreamerAudioOutput *gstAudioOutput = nullptr;
    QGstreamerVideoOutput *gstVideoOutput = nullptr;

    //    QGstElement streamSynchronizer;

    QHash<QByteArray, QGstPad> decoderOutputMap;
};

QT_END_NAMESPACE

#endif

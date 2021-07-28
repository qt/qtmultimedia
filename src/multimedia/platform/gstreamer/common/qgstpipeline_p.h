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

#ifndef qgstpipeline_p_H
#define qgstpipeline_p_H

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

#include <private/qtmultimediaglobal_p.h>
#include <QObject>

#include <private/qgst_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerMessage;

class QGstreamerSyncMessageFilter {
public:
    //returns true if message was processed and should be dropped, false otherwise
    virtual bool processSyncMessage(const QGstreamerMessage &message) = 0;
};


class QGstreamerBusMessageFilter {
public:
    //returns true if message was processed and should be dropped, false otherwise
    virtual bool processBusMessage(const QGstreamerMessage &message) = 0;
};

class QGstPipelinePrivate;

class QGstPipeline : public QGstBin
{
    QGstPipelinePrivate *d = nullptr;
public:
    constexpr QGstPipeline() = default;
    QGstPipeline(const QGstPipeline &o);
    QGstPipeline &operator=(const QGstPipeline &o);
    QGstPipeline(const char *name);
    QGstPipeline(GstPipeline *p);
    ~QGstPipeline();

    // This is needed to help us avoid sending QVideoFrames or audio buffers to the
    // application while we're prerolling the pipeline.
    // QMediaPlayer is still in a stopped state, while we put the gstreamer pipeline into a
    // Paused state so that we can get the required metadata of the stream and also have a fast
    // transition to play.
    QProperty<bool> *inStoppedState();

    void installMessageFilter(QGstreamerSyncMessageFilter *filter);
    void removeMessageFilter(QGstreamerSyncMessageFilter *filter);
    void installMessageFilter(QGstreamerBusMessageFilter *filter);
    void removeMessageFilter(QGstreamerBusMessageFilter *filter);

    GstPipeline *pipeline() const { return GST_PIPELINE_CAST(m_object); }

    void dumpGraph(const char *fileName)
    {
#if 1 //def QT_GST_CAPTURE_DEBUG
        GST_DEBUG_BIN_TO_DOT_FILE(bin(),
                                  GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_ALL |
                                                       GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES),
                                  fileName);
#else
        Q_UNUSED(fileName);
#endif
    }

    void flush() { seek(position(), m_rate); }

    bool seek(qint64 pos, double rate)
    {
        // always adjust the rate, so it can be  set before playback starts
        // setting position needs a loaded media file that's seekable
        m_rate = rate;
        bool success = gst_element_seek(element(), rate, GST_FORMAT_TIME,
                                        GstSeekFlags(GST_SEEK_FLAG_FLUSH),
                                        GST_SEEK_TYPE_SET, pos,
                                        GST_SEEK_TYPE_SET, -1);
        if (!success)
            return false;

        m_position = pos;
        return true;
    }
    bool setPlaybackRate(double rate)
    {
        if (rate == m_rate)
            return false;
        seek(position(), rate);
        return true;
    }
    double playbackRate() const { return m_rate; }

    bool setPosition(qint64 pos)
    {
        return seek(pos, m_rate);
    }
    qint64 position() const
    {
        gint64 pos;
        if (gst_element_query_position(element(), GST_FORMAT_TIME, &pos))
            m_position = pos;
        return m_position;
    }

    qint64 duration() const
    {
        gint64 d;
        if (!gst_element_query_duration(element(), GST_FORMAT_TIME, &d))
            return 0.;
        return d;
    }

private:
    mutable qint64 m_position = 0;
    double m_rate = 1.;
};

QT_END_NAMESPACE

#endif

/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include <private/qtmultimediaglobal_p.h>
#include <private/qgst_p.h>
#include <private/qgstpipeline_p.h>
#include <qwaitcondition.h>
#include <qmutex.h>
#include <qpointer.h>
#include <private/qgstreamervideosink_p.h>

QT_BEGIN_NAMESPACE

class QVideoSink;

class Q_MULTIMEDIA_EXPORT QGstreamerVideoOutput : public QObject
{
    Q_OBJECT

public:
    QGstreamerVideoOutput(QObject *parent = 0);
    ~QGstreamerVideoOutput();

    void setVideoSink(QVideoSink *sink);
    QGstreamerVideoSink *gstreamerVideoSink() const { return m_videoSink; }

    void setPipeline(const QGstPipeline &pipeline);

    QGstElement gstElement() const { return gstVideoOutput; }
    void linkSubtitleStream(QGstElement subtitleSrc);
    void unlinkSubtitleStream();

    void setIsPreview();
    void flushSubtitles();

private:
    void doLinkSubtitleStream();

    QPointer<QGstreamerVideoSink> m_videoSink;
    bool isFakeSink = true;

    // Gst elements
    QGstPipeline gstPipeline;

    QGstBin gstVideoOutput;
    QGstElement videoQueue;
    QGstElement videoConvert;
    QGstElement videoSink;

    QGstElement subtitleSrc;
    QGstElement subtitleSink;
};

QT_END_NAMESPACE

#endif

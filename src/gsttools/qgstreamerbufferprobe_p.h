/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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

#ifndef QGSTREAMERBUFFERPROBE_H
#define QGSTREAMERBUFFERPROBE_H

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

#include <private/qgsttools_global_p.h>
#include <gst/gst.h>

#include <QtCore/qglobal.h>


QT_BEGIN_NAMESPACE

class Q_GSTTOOLS_EXPORT QGstreamerBufferProbe
{
public:
    enum Flags
    {
        ProbeCaps       = 0x01,
        ProbeBuffers    = 0x02,
        ProbeAll    = ProbeCaps | ProbeBuffers
    };

    explicit QGstreamerBufferProbe(Flags flags = ProbeAll);
    virtual ~QGstreamerBufferProbe();

    void addProbeToPad(GstPad *pad, bool downstream = true);
    void removeProbeFromPad(GstPad *pad);

protected:
    virtual void probeCaps(GstCaps *caps);
    virtual bool probeBuffer(GstBuffer *buffer);

private:
#if GST_CHECK_VERSION(1,0,0)
    static GstPadProbeReturn capsProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn bufferProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    int m_capsProbeId = -1;
#else
    static gboolean bufferProbe(GstElement *element, GstBuffer *buffer, gpointer user_data);
    GstCaps *m_caps = nullptr;
#endif
    int m_bufferProbeId = -1;
    const Flags m_flags;
};

QT_END_NAMESPACE

#endif // QGSTREAMERAUDIOPROBECONTROL_H

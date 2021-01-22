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

#ifndef DIRECTSHOWSAMPLEGRABBER_H
#define DIRECTSHOWSAMPLEGRABBER_H

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>

// TODO: Fix this.
#include <directshowcameraglobal.h>

QT_BEGIN_NAMESPACE

class SampleGrabberCallbackPrivate;

class DirectShowSampleGrabber : public QObject
{
    Q_OBJECT
public:
    DirectShowSampleGrabber(QObject *p = nullptr);
    ~DirectShowSampleGrabber();

    // 0 = ISampleGrabberCB::SampleCB, 1 = ISampleGrabberCB::BufferCB
    enum class CallbackMethod : long
    {
        SampleCB,
        BufferCB
    };

    bool isValid() const { return m_filter && m_sampleGrabber; }
    bool getConnectedMediaType(AM_MEDIA_TYPE *mediaType);
    bool setMediaType(const AM_MEDIA_TYPE *mediaType);

    void stop();
    void start(CallbackMethod method, bool oneShot = false, bool bufferSamples = false);

    IBaseFilter *filter() const { return m_filter; }

Q_SIGNALS:
    void sampleAvailable(double time, IMediaSample *sample);
    void bufferAvailable(double time, const QByteArray &data);

private:
    IBaseFilter *m_filter = nullptr;
    ISampleGrabber *m_sampleGrabber = nullptr;
    SampleGrabberCallbackPrivate *m_sampleGabberCb = nullptr;
    CallbackMethod m_callbackType= CallbackMethod::BufferCB;
};

QT_END_NAMESPACE

#endif // DIRECTSHOWSAMPLEGRABBER_H

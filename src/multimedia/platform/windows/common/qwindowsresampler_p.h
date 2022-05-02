/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/


#ifndef QT_QWINDOWSRESAMPLER_H
#define QT_QWINDOWSRESAMPLER_H

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

#include <qbytearray.h>
#include <qbytearrayview.h>
#include <qaudioformat.h>
#include <private/qwindowsiupointer_p.h>
#include <qt_windows.h>

struct IMFSample;
struct IMFTransform;

QT_BEGIN_NAMESPACE

class QWindowsResampler
{
public:
    QWindowsResampler();
    ~QWindowsResampler();

    bool setup(const QAudioFormat &in, const QAudioFormat &out);

    QByteArray resample(const QByteArrayView &in);
    QByteArray resample(IMFSample *sample);

    QAudioFormat inputFormat() const { return m_inputFormat; }
    QAudioFormat outputFormat() const { return m_outputFormat; }

    quint64 outputBufferSize(quint64 inputBufferSize) const;
    quint64 inputBufferSize(quint64 outputBufferSize) const;

    quint64 totalInputBytes() const { return m_totalInputBytes; }
    quint64 totalOutputBytes() const { return m_totalOutputBytes; }

private:
    HRESULT processInput(const QByteArrayView &in);
    HRESULT processOutput(QByteArray &out);

    QWindowsIUPointer<IMFTransform> m_resampler;

    bool m_resamplerNeedsSampleBuffer = false;
    quint64 m_totalInputBytes = 0;
    quint64 m_totalOutputBytes = 0;
    QAudioFormat m_inputFormat;
    QAudioFormat m_outputFormat;

    DWORD m_inputStreamID = 0;
};

QT_END_NAMESPACE

#endif // QT_QWINDOWSRESAMPLER_H

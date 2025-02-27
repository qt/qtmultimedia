// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


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
#include <QtCore/private/qcomptr_p.h>
#include <private/qwindowsmediafoundation_p.h>
#include <private/qcominitializer_p.h>
#include <qt_windows.h>
#include <mftransform.h>

struct IMFSample;
struct IMFTransform;

QT_BEGIN_NAMESPACE

class QWindowsMediaFoundation;

class Q_MULTIMEDIA_EXPORT QWindowsResampler
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

    QComInitializer m_comInitializer;
    QWindowsMediaFoundation *m_wmf{ QWindowsMediaFoundation::instance() };
    QMFRuntimeInit m_wmfRuntime{ m_wmf };
    ComPtr<IMFTransform> m_resampler;

    bool m_resamplerNeedsSampleBuffer = false;
    quint64 m_totalInputBytes = 0;
    quint64 m_totalOutputBytes = 0;
    QAudioFormat m_inputFormat;
    QAudioFormat m_outputFormat;

    DWORD m_inputStreamID = 0;
};

QT_END_NAMESPACE

#endif // QT_QWINDOWSRESAMPLER_H

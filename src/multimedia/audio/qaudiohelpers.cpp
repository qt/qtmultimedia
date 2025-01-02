// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiohelpers_p.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

namespace QAudioHelperInternal
{

template<class T> void adjustSamples(qreal factor, const void *src, void *dst, int samples)
{
    const T *pSrc = (const T *)src;
    T *pDst = (T*)dst;
    for ( int i = 0; i < samples; i++ )
        pDst[i] = pSrc[i] * factor;
}

// Unsigned samples are biased around 0x80/0x8000 :/
// This makes a pure template solution a bit unwieldy but possible
template<class T> struct signedVersion {};
template<> struct signedVersion<quint8>
{
    using TS = qint8;
    static constexpr int offset = 0x80;
};

template<class T> void adjustUnsignedSamples(qreal factor, const void *src, void *dst, int samples)
{
    const T *pSrc = (const T *)src;
    T *pDst = (T*)dst;
    for ( int i = 0; i < samples; i++ ) {
        pDst[i] = signedVersion<T>::offset + ((typename signedVersion<T>::TS)(pSrc[i] - signedVersion<T>::offset) * factor);
    }
}

void qMultiplySamples(qreal factor, const QAudioFormat &format, const void* src, void* dest, int len)
{
    const int samplesCount = len / qMax(1, format.bytesPerSample());

    switch (format.sampleFormat()) {
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats:
        return;
    case QAudioFormat::UInt8:
        QAudioHelperInternal::adjustUnsignedSamples<quint8>(factor,src,dest,samplesCount);
        break;
    case QAudioFormat::Int16:
        QAudioHelperInternal::adjustSamples<qint16>(factor,src,dest,samplesCount);
        break;
    case QAudioFormat::Int32:
        QAudioHelperInternal::adjustSamples<qint32>(factor,src,dest,samplesCount);
        break;
    case QAudioFormat::Float:
        QAudioHelperInternal::adjustSamples<float>(factor,src,dest,samplesCount);
        break;
    }
}
}

QT_END_NAMESPACE

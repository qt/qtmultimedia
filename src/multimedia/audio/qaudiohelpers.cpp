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
    enum {offset = 0x80};
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

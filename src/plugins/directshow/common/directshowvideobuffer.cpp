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

#include "directshowvideobuffer.h"

QT_BEGIN_NAMESPACE

DirectShowVideoBuffer::DirectShowVideoBuffer(IMediaSample *sample, int bytesPerLine)
    : QAbstractVideoBuffer(NoHandle)
    , m_sample(sample)
    , m_bytesPerLine(bytesPerLine)
    , m_mapMode(NotMapped)
{
    m_sample->AddRef();
}

DirectShowVideoBuffer::~DirectShowVideoBuffer()
{
    m_sample->Release();
}

uchar *DirectShowVideoBuffer::map(MapMode mode, int *numBytes, int *bytesPerLine)
{
    if (m_mapMode == NotMapped && mode != NotMapped) {
        if (numBytes)
            *numBytes = m_sample->GetActualDataLength();

        if (bytesPerLine)
            *bytesPerLine = m_bytesPerLine;

        BYTE *bytes = nullptr;

        if (m_sample->GetPointer(&bytes) == S_OK) {
            m_mapMode = mode;

            return reinterpret_cast<uchar *>(bytes);
        }
    }
    return nullptr;
}

void DirectShowVideoBuffer::unmap()
{
    m_mapMode = NotMapped;
}

QAbstractVideoBuffer::MapMode DirectShowVideoBuffer::mapMode() const
{
    return m_mapMode;
}

QT_END_NAMESPACE

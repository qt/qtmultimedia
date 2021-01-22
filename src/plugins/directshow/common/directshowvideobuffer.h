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

#ifndef DIRECTSHOWVIDEOBUFFER_H
#define DIRECTSHOWVIDEOBUFFER_H

#include <dshow.h>

#include <qabstractvideobuffer.h>

QT_BEGIN_NAMESPACE

class DirectShowVideoBuffer : public QAbstractVideoBuffer
{
public:
    DirectShowVideoBuffer(IMediaSample *sample, int bytesPerLine);
    ~DirectShowVideoBuffer() override;

    IMediaSample *sample() { return m_sample; }

    uchar *map(MapMode mode, int *numBytes, int *bytesPerLine) override;
    void unmap() override;

    MapMode mapMode() const override;

private:
    IMediaSample *m_sample;
    int m_bytesPerLine;
    MapMode m_mapMode;
};

QT_END_NAMESPACE

#endif

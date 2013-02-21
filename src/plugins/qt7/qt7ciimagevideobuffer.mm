/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qt7ciimagevideobuffer.h"

#include <QuartzCore/CIFilter.h>
#include <QuartzCore/CIVector.h>

QT7CIImageVideoBuffer::QT7CIImageVideoBuffer(CIImage *image)
    : QAbstractVideoBuffer(CoreImageHandle)
    , m_image(image)
    , m_buffer(0)
    , m_mode(NotMapped)
{
    [m_image retain];
}

QT7CIImageVideoBuffer::~QT7CIImageVideoBuffer()
{
    [m_image release];
    [m_buffer release];
}

QAbstractVideoBuffer::MapMode QT7CIImageVideoBuffer::mapMode() const
{
    return m_mode;
}

uchar *QT7CIImageVideoBuffer::map(QAbstractVideoBuffer::MapMode mode, int *numBytes, int *bytesPerLine)
{
    if (mode == NotMapped || m_mode != NotMapped || !m_image)
        return 0;

    if (!m_buffer) {
        //swap R and B channels
        CIFilter *colorSwapFilter = [CIFilter filterWithName: @"CIColorMatrix"  keysAndValues:
                                     @"inputImage", m_image,
                                     @"inputRVector", [CIVector vectorWithX: 0  Y: 0  Z: 1  W: 0],
                                     @"inputGVector", [CIVector vectorWithX: 0  Y: 1  Z: 0  W: 0],
                                     @"inputBVector", [CIVector vectorWithX: 1  Y: 0  Z: 0  W: 0],
                                     @"inputAVector", [CIVector vectorWithX: 0  Y: 0  Z: 0  W: 1],
                                     @"inputBiasVector", [CIVector vectorWithX: 0  Y: 0  Z: 0  W: 0],
                                     nil];
        CIImage *img = [colorSwapFilter valueForKey: @"outputImage"];

        m_buffer = [[NSBitmapImageRep alloc] initWithCIImage:img];
    }

    if (numBytes)
        *numBytes = [m_buffer bytesPerPlane];

    if (bytesPerLine)
        *bytesPerLine = [m_buffer bytesPerRow];

    m_mode = mode;

    return [m_buffer bitmapData];
}

void QT7CIImageVideoBuffer::unmap()
{
    m_mode = NotMapped;
}

QVariant QT7CIImageVideoBuffer::handle() const
{
    return QVariant::fromValue<void*>(m_image);
}


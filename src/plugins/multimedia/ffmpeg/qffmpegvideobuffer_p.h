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

#ifndef QFFMPEGVIDEOBUFFER_P_H
#define QFFMPEGVIDEOBUFFER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtmultimediaglobal_p.h>
#include <private/qabstractvideobuffer_p.h>
#include <qvideoframe.h>
#include <QtCore/qvariant.h>

#include "qffmpeg_p.h"
#include "qffmpeghwaccel_p.h"

QT_BEGIN_NAMESPACE

class QFFmpegVideoBuffer : public QAbstractVideoBuffer
{
public:

    QFFmpegVideoBuffer(AVFrame *frame);
    ~QFFmpegVideoBuffer();

    QVideoFrame::MapMode mapMode() const override;
    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    virtual void mapTextures() override;
    virtual quint64 textureHandle(int plane) const override;

    QVideoFrameFormat::PixelFormat pixelFormat() const;
    QSize size() const;

    static QVideoFrameFormat::PixelFormat toQtPixelFormat(AVPixelFormat avPixelFormat, bool *needsConversion = nullptr);
    static AVPixelFormat toAVPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat);

    void convertSWFrame();

    AVFrame *getHWFrame() const { return hwFrame; }

    void setTextureConverter(const QFFmpeg::TextureConverter &converter);

    QVideoFrameFormat::YCbCrColorSpace colorSpace() const;
    QVideoFrameFormat::ColorTransfer colorTransfer() const;

private:
    QVideoFrameFormat::PixelFormat m_pixelFormat;
    AVFrame *frame = nullptr;
    AVFrame *hwFrame = nullptr;
    AVFrame *swFrame = nullptr;
    QFFmpeg::TextureConverter textureConverter;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    QFFmpeg::TextureSet *textures = nullptr;
};

QT_END_NAMESPACE

#endif

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
#ifndef QFFMPEGHWACCEL_P_H
#define QFFMPEGHWACCEL_P_H

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

#include "qffmpeg_p.h"

#include <qshareddata.h>

QT_BEGIN_NAMESPACE

class QRhi;

namespace QFFmpeg {

// used for the get_format callback for the decoder
enum AVPixelFormat getFormat(struct AVCodecContext *s, const enum AVPixelFormat * fmt);

class HWAccel;

class HWAccelBackend
{
    friend class HWAccel;
protected:
    HWAccelBackend(AVBufferRef *hwContext);
public:
    virtual ~HWAccelBackend();
    virtual void setRhi(QRhi *) {}
    virtual bool getTextures(AVFrame */*frame*/, qint64 */*textures*/) { return false; }
    virtual void freeTextures(qint64 */*textures*/) {}
    virtual AVPixelFormat format(AVFrame *frame) const;

    QAtomicInt ref = 0;
    AVBufferRef *hwContext = nullptr;
    QRhi *rhi = nullptr;
};

class HWAccel
{
public:
    HWAccel() = default;
    explicit HWAccel(AVCodec *codec);
    ~HWAccel() = default;

    QRhi *rhi() const { return d ? d->rhi : nullptr; }
    void setRhi(QRhi *rhi) {
        if (d)
            d->setRhi(rhi);
    }
    bool getTextures(AVFrame *frame, qint64 *textures) {
        if (!d)
            return false;
        return d->getTextures(frame, textures);
    }
    void freeTextures(qint64 *textures) {
        if (d)
            d->freeTextures(textures);
    }
    AVPixelFormat format(AVFrame *frame) const
    {
        return d ? d->format(frame) : AVPixelFormat(frame->format);
    }
    bool canProvideTextures() const { return d->rhi != nullptr; }
    AVBufferRef *hwContext() const { return d ? d->hwContext : nullptr; }
    bool isNull() const { return !d; }

private:
    QExplicitlySharedDataPointer<HWAccelBackend> d;
};
}

QT_END_NAMESPACE

#endif

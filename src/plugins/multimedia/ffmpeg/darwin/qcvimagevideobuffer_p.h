// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCVIMAGEVIDEOBUFFER_P_H
#define QCVIMAGEVIDEOBUFFER_P_H

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

#include <QtCore/qtconfigmacros.h>

#include <QtMultimedia/private/qavfhelpers_p.h>
#include <QtMultimedia/qabstractvideobuffer.h>

#include <CoreVideo/CoreVideo.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class CVImageVideoBuffer : public QAbstractVideoBuffer
{
public:
    explicit CVImageVideoBuffer(QAVFHelpers::QSharedCVPixelBuffer &&);

    ~CVImageVideoBuffer();

    CVImageVideoBuffer::MapData map(QVideoFrame::MapMode mode) override;

    void unmap() override;

    QVideoFrameFormat format() const override { return {}; }

private:
    QAVFHelpers::QSharedCVPixelBuffer m_buffer;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
};

}

QT_END_NAMESPACE

#endif // QCVIMAGEVIDEOBUFFER_P_H

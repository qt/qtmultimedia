// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFIMAGECAPTURE_H
#define QAVFIMAGECAPTURE_H

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

#include <QtFFmpegMediaPluginImpl/private/qffmpegimagecapture_p.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class QAVFImageCapture : public QFFmpegImageCapture
{
public:
    QAVFImageCapture(QImageCapture *parent);
    ~QAVFImageCapture() override = default;

protected:
    void setupVideoSourceConnections() override;
    int doCapture(const QString &fileName) override;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QAVFIMAGECAPTURE_H

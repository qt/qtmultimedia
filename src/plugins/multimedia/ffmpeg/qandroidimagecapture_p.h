// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR
// GPL-3.0-only

#ifndef QANDROIDIMAGECAPTURE_H
#define QANDROIDIMAGECAPTURE_H

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

#include "qffmpegimagecapture_p.h"

QT_BEGIN_NAMESPACE

class QAndroidImageCapture : public QFFmpegImageCapture
{
public:
    QAndroidImageCapture(QImageCapture *parent);
    ~QAndroidImageCapture() override;

protected:
    void setupVideoSourceConnections() override;
    int doCapture(const QString &fileName) override;

private slots:
    void updateExif(int id, const QString &filename);
};

QT_END_NAMESPACE

#endif // QANDROIDIMAGECAPTURE_H

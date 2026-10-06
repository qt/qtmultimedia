// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEDIAENGINEVIDEOSINK_P_H
#define QMEDIAENGINEVIDEOSINK_P_H

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

#include <QtMultimedia/private/qplatformvideosink_p.h>

QT_BEGIN_NAMESPACE

class QMediaEngineVideoSink : public QPlatformVideoSink
{
    Q_OBJECT
public:
    explicit QMediaEngineVideoSink(QVideoSink *parent);
    ~QMediaEngineVideoSink() override;
};

QT_END_NAMESPACE

#endif // QMEDIAENGINEVIDEOSINK_P_H

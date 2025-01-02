// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDARWINFORMATINFO_H
#define QDARWINFORMATINFO_H

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

#include <private/qplatformmediaformatinfo_p.h>
#include <qlist.h>

QT_BEGIN_NAMESPACE

class QDarwinMediaDevices;

class QDarwinFormatInfo : public QPlatformMediaFormatInfo
{
public:
    QDarwinFormatInfo();
    ~QDarwinFormatInfo();

    static int audioFormatForCodec(QMediaFormat::AudioCodec codec);
    static NSString *videoFormatForCodec(QMediaFormat::VideoCodec codec);
    static NSString *avFileTypeForContainerFormat(QMediaFormat::FileFormat fileType);
};

QT_END_NAMESPACE

#endif

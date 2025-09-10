// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXFORMATINFO_H
#define QQNXFORMATINFO_H

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
#include <qhash.h>
#include <qlist.h>

QT_BEGIN_NAMESPACE

class QQnxFormatInfo : public QPlatformMediaFormatInfo
{
public:
    QQnxFormatInfo();
    ~QQnxFormatInfo();
};

QT_END_NAMESPACE

#endif

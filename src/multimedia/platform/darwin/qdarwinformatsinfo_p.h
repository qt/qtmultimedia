/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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

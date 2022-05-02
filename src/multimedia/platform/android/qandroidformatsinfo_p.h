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

#ifndef QANDROIDFORMATINFO_H
#define QANDROIDFORMATINFO_H

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

QT_BEGIN_NAMESPACE

class QAndroidFormatInfo : public QPlatformMediaFormatInfo
{
public:
    QAndroidFormatInfo();
    ~QAndroidFormatInfo();

private:
    QMediaFormat::AudioCodec hasEncoder(QMediaFormat::AudioCodec codec) const;
    QMediaFormat::VideoCodec hasEncoder(QMediaFormat::VideoCodec codec) const;
    QMediaFormat::AudioCodec hasDecoder(QMediaFormat::AudioCodec codec) const;
    QMediaFormat::VideoCodec hasDecoder(QMediaFormat::VideoCodec codec) const;

    QStringList m_supportedDecoders;
    QStringList m_supportedEncoders;
};

QT_END_NAMESPACE

#endif

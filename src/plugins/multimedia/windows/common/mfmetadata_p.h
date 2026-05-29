// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MFMETADATACONTROL_H
#define MFMETADATACONTROL_H

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

#include <qmediametadata.h>
#include "mfidl.h"


struct IMFMetadata;

class MFMetaData
{
public:
    static QMediaMetaData fromNative(IMFMediaSource* mediaSource);
    static QMediaMetaData fromNative(IPropertyStore *content);
    static QMediaMetaData fromNative(IMFMetadata *metadata);
    static void toNative(const QMediaMetaData &metaData, IPropertyStore *content);
};

#endif

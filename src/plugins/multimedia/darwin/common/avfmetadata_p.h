// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFMEDIAPLAYERMETADATACONTROL_H
#define AVFMEDIAPLAYERMETADATACONTROL_H

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

#include <QtMultimedia/QMediaMetaData>
#include <QtCore/qvariant.h>

#import <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFMediaPlayer;

class AVFMetaData
{
public:
    static QMediaMetaData fromAsset(AVAsset *asset);
    static QMediaMetaData fromAssetTrack(AVAssetTrack *asset);
    static NSMutableArray<AVMetadataItem *> *toAVMetadataForFormat(QMediaMetaData metaData, AVFileType format);
};

QT_END_NAMESPACE

#endif // AVFMEDIAPLAYERMETADATACONTROL_H

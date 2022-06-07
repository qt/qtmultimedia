// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDMETADATA_H
#define QANDROIDMETADATA_H

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
#include <qurl.h>
#include <QMutex>
#include <QVariant>

QT_BEGIN_NAMESPACE

class AndroidMediaMetadataRetriever;

class QAndroidMetaData : public QMediaMetaData
{
public:
    static QMediaMetaData extractMetadata(const QUrl &url);

    QAndroidMetaData(int trackType, int androidTrackType, int androidTrackNumber,
                     const QString &mimeType, const QString &language);

    int trackType() const;
    int androidTrackType() const;
    int androidTrackNumber() const;

private:
    int mTrackType;
    int mAndroidTrackType;
    int mAndroidTrackNumber;
};

QT_END_NAMESPACE

#endif // QANDROIDMETADATA_H

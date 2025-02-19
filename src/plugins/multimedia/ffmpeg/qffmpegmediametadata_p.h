// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGMEDIAMETADATA_H
#define QFFMPEGMEDIAMETADATA_H

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
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>

QT_BEGIN_NAMESPACE

class QFFmpegMetaData : public QMediaMetaData
{
public:
    static void addEntry(QMediaMetaData &metaData, AVDictionaryEntry *entry);
    static QMediaMetaData fromAVMetaData(const AVDictionary *tags);

    static QByteArray value(const QMediaMetaData &metaData, QMediaMetaData::Key key);
    static AVDictionary *toAVMetaData(const QMediaMetaData &metaData);
};

QT_END_NAMESPACE

#endif // QFFMPEGMEDIAMETADATA_H

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERMETADATA_H
#define QGSTREAMERMETADATA_H

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
#include <qvariant.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class QGstreamerMetaData : public QMediaMetaData
{
public:
    static QGstreamerMetaData fromGstTagList(const GstTagList *tags);
    GstTagList *toGstTagList() const;

    void setMetaData(GstBin *bin) const;
    void setMetaData(GstElement *element) const;
};

QT_END_NAMESPACE

#endif // QGSTREAMERMETADATA_H

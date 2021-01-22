/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QGSTCODECSINFO_H
#define QGSTCODECSINFO_H

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

#include <private/qgsttools_global_p.h>
#include <QtCore/qmap.h>
#include <QtCore/qstringlist.h>
#include <QSet>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class Q_GSTTOOLS_EXPORT QGstCodecsInfo
{
public:
    enum ElementType { AudioEncoder, VideoEncoder, Muxer };

    struct CodecInfo {
        QString description;
        QByteArray elementName;
        GstRank rank;
    };

    QGstCodecsInfo(ElementType elementType);

    QStringList supportedCodecs() const;
    QString codecDescription(const QString &codec) const;
    QByteArray codecElement(const QString &codec) const;
    QStringList codecOptions(const QString &codec) const;
    QSet<QString> supportedStreamTypes(const QString &codec) const;
    QStringList supportedCodecs(const QSet<QString> &types) const;

private:
    void updateCodecs(ElementType elementType);
    GList *elementFactories(ElementType elementType) const;

    QStringList m_codecs;
    QMap<QString, CodecInfo> m_codecInfo;
    QMap<QString, QSet<QString>> m_streamTypes;
};

Q_DECLARE_TYPEINFO(QGstCodecsInfo::CodecInfo, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif

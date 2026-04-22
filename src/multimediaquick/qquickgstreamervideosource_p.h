// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKGSTREAMERVIDEOSOURCE_P_H
#define QQUICKGSTREAMERVIDEOSOURCE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qgstreamervideosource.h>
#include <QtMultimediaQuick/private/qtmultimediaquickglobal_p.h>
#include <QtQml/qqml.h>

#include <optional>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIAQUICK_EXPORT QQuickGStreamerVideoSource : public QGStreamerVideoSource
{
    Q_OBJECT
    Q_PROPERTY(bool active READ qmlIsActive WRITE qmlSetActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(QString gstBinDescription READ gstBinDescription WRITE setQmlGstBinDescription
                       REQUIRED FINAL)
    QML_NAMED_ELEMENT(GStreamerVideoSource)

public:
    explicit QQuickGStreamerVideoSource(QObject *parent = nullptr);

    bool qmlIsActive() const;
    void qmlSetActive(bool active);
    void setQmlGstBinDescription(QString gstBinDescription);

private:
    struct PendingProperties
    {
        bool active = false;
    };

    std::optional<PendingProperties> m_pendingProperties = PendingProperties{};
};

QT_END_NAMESPACE

#endif // QQUICKGSTREAMERVIDEOSOURCE_P_H

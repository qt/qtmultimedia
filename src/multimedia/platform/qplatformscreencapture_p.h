// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMSCREENCAPTURE_H
#define QPLATFORMSCREENCAPTURE_H

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

#include <QtCore/qobject.h>
#include <QtCore/qnativeinterface.h>
#include <QtCore/private/qglobal_p.h>

#include <QtMultimedia/qtmultimediaglobal.h>

#include "qscreencapture.h"
#include "qvideoframeformat.h"

#include <optional>

QT_BEGIN_NAMESPACE

class QVideoFrame;

class Q_MULTIMEDIA_EXPORT QPlatformScreenCapture : public QObject
{
    Q_OBJECT

public:
    explicit QPlatformScreenCapture(QScreenCapture *screenCapture);

    virtual void setActive(bool active) = 0;
    virtual bool isActive() const = 0;

    virtual void setScreen(QScreen *s) = 0;
    virtual QScreen *screen() const = 0;

    virtual void setWindow(QWindow *w);
    virtual QWindow *window() const;

    virtual void setWindowId(WId id);
    virtual WId windowId() const;

    virtual QScreenCapture::Error error() const;
    virtual QString errorString() const;

    virtual QVideoFrameFormat format() const = 0;

    QScreenCapture *screenCapture() const;

    virtual std::optional<int> ffmpegHWPixelFormat() const;

public Q_SLOTS:
    void updateError(QScreenCapture::Error error, const QString &errorString);

Q_SIGNALS:
    void newVideoFrame(const QVideoFrame &);

private:
    QScreenCapture::Error m_error = QScreenCapture::NoError;
    QString m_errorString;
    QScreenCapture *m_screenCapture = nullptr;
};

QT_END_NAMESPACE

#endif // QPLATFORMSCREENCAPTURE_H

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

QT_BEGIN_NAMESPACE

class QVideoFrame;

class Q_MULTIMEDIA_EXPORT QPlatformScreenCapture : public QObject
{
    Q_OBJECT

public:
    QPlatformScreenCapture(QScreenCapture *screenCapture)
        : QObject(screenCapture), m_screenCapture(screenCapture)
    {}

    virtual void setActive(bool active) = 0;
    virtual bool isActive() const = 0;

    virtual void setScreen(QScreen *s) = 0;
    virtual QScreen *screen() const = 0;

    virtual void setWindow(QWindow *w) {
        if (w) {
            emit m_screenCapture->errorOccurred(QScreenCapture::WindowCapturingNotSupported,
                                                QLatin1String("Window capture is not supported"));
        }
    }

    virtual QWindow *window() const { return nullptr; }

    virtual void setWindowId(WId id) {
        if (id) {
            emit m_screenCapture->errorOccurred(QScreenCapture::WindowCapturingNotSupported,
                                                QLatin1String("Window capture is not supported"));
        }
    }

    virtual WId windowId() const { return 0; }

    virtual QScreenCapture::Error error() const { return m_error; }
    virtual QString errorString() const { return m_errorString; }

    virtual QVideoFrameFormat format() const = 0;

    QScreenCapture *screenCapture() const { return m_screenCapture; }

public Q_SLOTS:
    void updateError(QScreenCapture::Error error, const QString &errorString)
    {
        bool changed = error != m_error || errorString != m_errorString;
        m_error = error;
        m_errorString = errorString;
        if (changed) {
            if (m_error != QScreenCapture::NoError)
                emit m_screenCapture->errorOccurred(error, errorString);
            emit m_screenCapture->errorChanged();
        }
    }

Q_SIGNALS:
    void newVideoFrame(const QVideoFrame &);

private:
    QScreenCapture::Error m_error = QScreenCapture::NoError;
    QString m_errorString;
    QScreenCapture *m_screenCapture = nullptr;
};

QT_END_NAMESPACE

#endif // QPLATFORMSCREENCAPTURE_H

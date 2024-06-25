// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMSCREENCAPTURE_H
#define QWASMSCREENCAPTURE_H

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

#include <QObject>
#include <private/qplatformsurfacecapture_p.h>
#include <QScreenCapture>

#include <QtCore/qloggingcategory.h>
#include "qwasmvideooutput_p.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qWasmScreenCapture)

class QWasmMediaCaptureSession;

class QWasmScreenCapture : public QPlatformSurfaceCapture
{
    Q_OBJECT
public:
    explicit QWasmScreenCapture(QScreenCapture *screenCap);
    ~QWasmScreenCapture();

    QVideoFrameFormat frameFormat() const override;
    void setCaptureSession(QPlatformMediaCaptureSession *session) override;
    void setVideoStream(emscripten::val stream);
    void setVideoOutput(QWasmVideoOutput *output) ;

protected:
    bool setActiveInternal(bool) override;

private:
    QWasmMediaCaptureSession *m_captureSession = nullptr;
    int m_lastId = 0;
    QWasmVideoOutput *m_videoOutput = nullptr;
    QScreenCapture *m_screenCapture = nullptr;
};

QT_END_NAMESPACE
#endif // QWASMSCREENCAPTURE_H

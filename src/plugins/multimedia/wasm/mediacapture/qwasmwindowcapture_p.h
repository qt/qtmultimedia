// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMWINDOWCAPTURE_H
#define QWASMWINDOWCAPTURE_H

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
#include <QtCore/qloggingcategory.h>
#include "qwasmvideooutput_p.h"
#include "mediacapture/qwasmwindowcapture_p.h"
#include <QWindowCapture>

#include <QScreenCapture>

#include "private/qplatformcapturablewindows_p.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qWasmImageCapture)

class QWasmMediaCaptureSession;

class QWasmWindowCapture : public QPlatformSurfaceCapture
{
    Q_OBJECT
public:
    explicit QWasmWindowCapture(QWindowCapture *winCap);
    ~QWasmWindowCapture();

    QVideoFrameFormat frameFormat() const override;
    void setVideoStream(emscripten::val stream);
    void setCaptureSession(QPlatformMediaCaptureSession *session) override;
    void setVideoOutput(QWasmVideoOutput *object);
protected:
    bool setActiveInternal(bool) override;

private:
    QWasmMediaCaptureSession *m_captureSession = nullptr;
    QWasmVideoOutput *m_videoOutput = nullptr;


};

QT_END_NAMESPACE
#endif // QWASMWINDOWCAPTURE_H

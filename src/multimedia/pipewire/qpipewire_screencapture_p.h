// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_SCREENCAPTURE_P_H
#define QPIPEWIRE_SCREENCAPTURE_P_H

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

#include <QtMultimedia/private/qplatformsurfacecapture_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class QPipeWireCaptureHelper;
class QPipeWireInstance;

class Q_MULTIMEDIA_EXPORT QPipeWireCapture : public QPlatformSurfaceCapture
{
    Q_OBJECT

public:
    QPipeWireCapture(Source initialSource, std::shared_ptr<QPipeWireInstance>);
    ~QPipeWireCapture() override;

    QVideoFrameFormat frameFormat() const override;

    static bool isSupported();

    static std::unique_ptr<QPipeWireCapture> create(Source initialSource = {});

protected:
    bool setActiveInternal(bool active) override;

private:
    const std::shared_ptr<QPipeWireInstance> m_instance;
    std::unique_ptr<QPipeWireCaptureHelper> m_helper;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_SCREENCAPTURE_P_H

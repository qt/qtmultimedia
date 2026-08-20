// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_screencapture_p.h"

#include <QtMultimedia/private/qpipewire_screencapturehelper_p.h>
#include <QtMultimedia/private/qpipewire_instance_p.h>

#include <utility>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QtPipeWire {

QPipeWireCapture::QPipeWireCapture(Source initialSource)
    : QPlatformSurfaceCapture(std::move(initialSource))
{
}

QPipeWireCapture::~QPipeWireCapture() = default;

bool QPipeWireCapture::isSupported()
{
    if (!QPipeWireInstance::isLoaded())
        return false;

    if (auto check = QPipeWireInstance::checkSupportPluginsLoadable(); !check) {
        qCWarning(lcPipewire) << "PipeWire support plugins could not be loaded:"
                              << check.error().message();
        return false;
    }

    return QPipeWireCaptureHelper::isSupported();
}

QVideoFrameFormat QPipeWireCapture::frameFormat() const
{
    if (m_helper)
        return m_helper->frameFormat();

    return QVideoFrameFormat();
}

bool QPipeWireCapture::setActiveInternal(bool active)
{
    // Initialize helper, keep alive between captures
    if (!m_helper)
        m_helper = std::make_unique<QPipeWireCaptureHelper>(*this);

    if (!QPipeWireCaptureHelper::isSupported()) {
        updateError(QPlatformSurfaceCapture::Error::InternalError,
                    u"There is no ScreenCast service available in org.freedesktop.portal!"_s);
        return false;
    }

    m_helper->setFrameRate(frameRate().value_or(DefaultCaptureFrameRate));

    if (active)
        m_helper->start();
    else
        m_helper->stop();

    return true;
}

} // namespace QtPipeWire

QT_END_NAMESPACE

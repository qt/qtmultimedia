// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "evrvideowindowcontrol_p.h"

QT_BEGIN_NAMESPACE

EvrVideoWindowControl::EvrVideoWindowControl(QVideoSink *parent)
    : QPlatformVideoSink(parent)
    , m_windowId(0)
    , m_windowColor(RGB(0, 0, 0))
    , m_dirtyValues(0)
    , m_aspectRatioMode(Qt::KeepAspectRatio)
    , m_brightness(0)
    , m_contrast(0)
    , m_hue(0)
    , m_saturation(0)
    , m_fullScreen(false)
    , m_displayControl(0)
    , m_processor(0)
{
}

EvrVideoWindowControl::~EvrVideoWindowControl()
{
   clear();
}

bool EvrVideoWindowControl::setEvr(IUnknown *evr)
{
    clear();

    if (!evr)
        return true;

    IMFGetService *service = NULL;

    if (SUCCEEDED(evr->QueryInterface(IID_PPV_ARGS(&service)))
            && SUCCEEDED(service->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&m_displayControl)))) {

        service->GetService(MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&m_processor));

        setWinId(m_windowId);
        setDisplayRect(m_displayRect);
        setAspectRatioMode(m_aspectRatioMode);
        m_dirtyValues = DXVA2_ProcAmp_Brightness | DXVA2_ProcAmp_Contrast | DXVA2_ProcAmp_Hue | DXVA2_ProcAmp_Saturation;
        applyImageControls();
    }

    if (service)
        service->Release();

    return m_displayControl != NULL;
}

void EvrVideoWindowControl::clear()
{
    if (m_displayControl)
        m_displayControl->Release();
    m_displayControl = NULL;

    if (m_processor)
        m_processor->Release();
    m_processor = NULL;
}

void EvrVideoWindowControl::setWinId(WId id)
{
    m_windowId = id;

    if (m_displayControl)
        m_displayControl->SetVideoWindow(HWND(m_windowId));
}

void EvrVideoWindowControl::setDisplayRect(const QRect &rect)
{
    m_displayRect = rect;

    if (m_displayControl) {
        RECT displayRect = { rect.left(), rect.top(), rect.right() + 1, rect.bottom() + 1 };
        QSize sourceSize = nativeSize();

        RECT sourceRect = { 0, 0, sourceSize.width(), sourceSize.height() };

        if (m_aspectRatioMode == Qt::KeepAspectRatioByExpanding) {
            QSize clippedSize = rect.size();
            clippedSize.scale(sourceRect.right, sourceRect.bottom, Qt::KeepAspectRatio);

            sourceRect.left = (sourceRect.right - clippedSize.width()) / 2;
            sourceRect.top = (sourceRect.bottom - clippedSize.height()) / 2;
            sourceRect.right = sourceRect.left + clippedSize.width();
            sourceRect.bottom = sourceRect.top + clippedSize.height();
        }

        if (sourceSize.width() > 0 && sourceSize.height() > 0) {
            MFVideoNormalizedRect sourceNormRect;
            sourceNormRect.left = float(sourceRect.left) / float(sourceRect.right);
            sourceNormRect.top = float(sourceRect.top) / float(sourceRect.bottom);
            sourceNormRect.right = float(sourceRect.right) / float(sourceRect.right);
            sourceNormRect.bottom = float(sourceRect.bottom) / float(sourceRect.bottom);
            m_displayControl->SetVideoPosition(&sourceNormRect, &displayRect);
        } else {
            m_displayControl->SetVideoPosition(NULL, &displayRect);
        }
    }
}

void EvrVideoWindowControl::setFullScreen(bool fullScreen)
{
    if (m_fullScreen == fullScreen)
        return;
}

void EvrVideoWindowControl::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_aspectRatioMode = mode;

    if (m_displayControl) {
        switch (mode) {
        case Qt::IgnoreAspectRatio:
            //comment from MSDN: Do not maintain the aspect ratio of the video. Stretch the video to fit the output rectangle.
            m_displayControl->SetAspectRatioMode(MFVideoARMode_None);
            break;
        case Qt::KeepAspectRatio:
            //comment from MSDN: Preserve the aspect ratio of the video by letterboxing or within the output rectangle.
            m_displayControl->SetAspectRatioMode(MFVideoARMode_PreservePicture);
            break;
        case Qt::KeepAspectRatioByExpanding:
            //for this mode, more adjustment will be done in setDisplayRect
            m_displayControl->SetAspectRatioMode(MFVideoARMode_PreservePicture);
            break;
        default:
            break;
        }
        setDisplayRect(m_displayRect);
    }
}

void EvrVideoWindowControl::setBrightness(float brightness)
{
    if (m_brightness == brightness)
        return;

    m_brightness = brightness;

    m_dirtyValues |= DXVA2_ProcAmp_Brightness;

    applyImageControls();
}

void EvrVideoWindowControl::setContrast(float contrast)
{
    if (m_contrast == contrast)
        return;

    m_contrast = contrast;

    m_dirtyValues |= DXVA2_ProcAmp_Contrast;

    applyImageControls();
}

void EvrVideoWindowControl::setHue(float hue)
{
    if (m_hue == hue)
        return;

    m_hue = hue;

    m_dirtyValues |= DXVA2_ProcAmp_Hue;

    applyImageControls();
}

void EvrVideoWindowControl::setSaturation(float saturation)
{
    if (m_saturation == saturation)
        return;

    m_saturation = saturation;

    m_dirtyValues |= DXVA2_ProcAmp_Saturation;

    applyImageControls();
}

void EvrVideoWindowControl::applyImageControls()
{
    if (m_processor) {
        DXVA2_ProcAmpValues values;
        if (m_dirtyValues & DXVA2_ProcAmp_Brightness) {
            values.Brightness = scaleProcAmpValue(DXVA2_ProcAmp_Brightness, m_brightness);
        }
        if (m_dirtyValues & DXVA2_ProcAmp_Contrast) {
            values.Contrast = scaleProcAmpValue(DXVA2_ProcAmp_Contrast, m_contrast);
        }
        if (m_dirtyValues & DXVA2_ProcAmp_Hue) {
            values.Hue = scaleProcAmpValue(DXVA2_ProcAmp_Hue, m_hue);
        }
        if (m_dirtyValues & DXVA2_ProcAmp_Saturation) {
            values.Saturation = scaleProcAmpValue(DXVA2_ProcAmp_Saturation, m_saturation);
        }

        if (SUCCEEDED(m_processor->SetProcAmpValues(m_dirtyValues, &values))) {
            m_dirtyValues = 0;
        }
    }
}

DXVA2_Fixed32 EvrVideoWindowControl::scaleProcAmpValue(DWORD prop, float value) const
{
    float scaledValue = 0.0;

    DXVA2_ValueRange  range;
    if (SUCCEEDED(m_processor->GetProcAmpRange(prop, &range))) {
        scaledValue = DXVA2FixedToFloat(range.DefaultValue);
        if (value > 0)
            scaledValue += float(value) * (DXVA2FixedToFloat(range.MaxValue) - DXVA2FixedToFloat(range.DefaultValue));
        else if (value < 0)
            scaledValue -= float(value) * (DXVA2FixedToFloat(range.MinValue) - DXVA2FixedToFloat(range.DefaultValue));
    }

    return DXVA2FloatToFixed(scaledValue);
}

QT_END_NAMESPACE

#include "moc_evrvideowindowcontrol_p.cpp"

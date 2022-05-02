/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include "qwasmmediaintegration_p.h"
#include "qwasmmediadevices_p.h"
#include <QLoggingCategory>

#include <private/qplatformmediaformatinfo_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qtWasmMediaPlugin, "qt.multimedia.wasm")

QWasmMediaIntegration::QWasmMediaIntegration()
{

}

QWasmMediaIntegration::~QWasmMediaIntegration()
{
    delete m_devices;
    delete m_formatInfo;
}

QPlatformMediaFormatInfo *QWasmMediaIntegration::formatInfo()
{
     if (!m_formatInfo)
         m_formatInfo = new QPlatformMediaFormatInfo();
     return m_formatInfo;
}

QPlatformMediaDevices *QWasmMediaIntegration::devices()
{
    if (!m_devices)
        m_devices = new QWasmMediaDevices();
    return m_devices;
}

QT_END_NAMESPACE

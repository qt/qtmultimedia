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

#include "qgstreamervideoinputdevicecontrol_p.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>

#include <private/qgstutils_p.h>

QGstreamerVideoInputDeviceControl::QGstreamerVideoInputDeviceControl(QObject *parent)
    : QVideoDeviceSelectorControl(parent)
{
}

QGstreamerVideoInputDeviceControl::QGstreamerVideoInputDeviceControl(
        GstElementFactory *factory, QObject *parent)
    : QVideoDeviceSelectorControl(parent)
    , m_factory(factory)
{
    if (m_factory)
        gst_object_ref(GST_OBJECT(m_factory));
}

QGstreamerVideoInputDeviceControl::~QGstreamerVideoInputDeviceControl()
{
    if (m_factory)
        gst_object_unref(GST_OBJECT(m_factory));
}

int QGstreamerVideoInputDeviceControl::deviceCount() const
{
    return QGstUtils::enumerateCameras(m_factory).count();
}

QString QGstreamerVideoInputDeviceControl::deviceName(int index) const
{
    return QGstUtils::enumerateCameras(m_factory).value(index).name;
}

QString QGstreamerVideoInputDeviceControl::deviceDescription(int index) const
{
    return QGstUtils::enumerateCameras(m_factory).value(index).description;
}

int QGstreamerVideoInputDeviceControl::defaultDevice() const
{
    return 0;
}

int QGstreamerVideoInputDeviceControl::selectedDevice() const
{
    return m_selectedDevice;
}

void QGstreamerVideoInputDeviceControl::setSelectedDevice(int index)
{
    // Always update selected device and proxy it to clients
    m_selectedDevice = index;
    emit selectedDeviceChanged(index);
    emit selectedDeviceChanged(deviceName(index));
}

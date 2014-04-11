/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstreamervideoinputdevicecontrol_p.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>

#include <private/qcore_unix_p.h>
#include <linux/videodev2.h>

QGstreamerVideoInputDeviceControl::QGstreamerVideoInputDeviceControl(QObject *parent)
    :QVideoDeviceSelectorControl(parent), m_source(0), m_selectedDevice(0)
{
    update();
}

QGstreamerVideoInputDeviceControl::QGstreamerVideoInputDeviceControl(GstElement *source, QObject *parent)
    :QVideoDeviceSelectorControl(parent), m_source(source), m_selectedDevice(0)
{
    if (m_source)
        gst_object_ref(GST_OBJECT(m_source));

    update();
}

QGstreamerVideoInputDeviceControl::~QGstreamerVideoInputDeviceControl()
{
    if (m_source)
        gst_object_unref(GST_OBJECT(m_source));
}

int QGstreamerVideoInputDeviceControl::deviceCount() const
{
    return m_names.size();
}

QString QGstreamerVideoInputDeviceControl::deviceName(int index) const
{
    return m_names[index];
}

QString QGstreamerVideoInputDeviceControl::deviceDescription(int index) const
{
    return m_descriptions[index];
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
    if (index != m_selectedDevice) {
        m_selectedDevice = index;
        emit selectedDeviceChanged(index);
        emit selectedDeviceChanged(deviceName(index));
    }
}


void QGstreamerVideoInputDeviceControl::update()
{
    m_names.clear();
    m_descriptions.clear();

    // subdevsrc and the like have a camera-device property that takes an enumeration
    // identifying a primary or secondary camera, so return identifiers that map to those
    // instead of a list of actual devices.
    if (m_source && g_object_class_find_property(G_OBJECT_GET_CLASS(m_source), "camera-device")) {
        m_names << QLatin1String("primary") << QLatin1String("secondary");
        m_descriptions << tr("Main camera") << tr("Front camera");
        return;
    }

    QDir devDir("/dev");
    devDir.setFilter(QDir::System);

    QFileInfoList entries = devDir.entryInfoList(QStringList() << "video*");

    foreach( const QFileInfo &entryInfo, entries ) {
        //qDebug() << "Try" << entryInfo.filePath();

        int fd = qt_safe_open(entryInfo.filePath().toLatin1().constData(), O_RDWR );
        if (fd == -1)
            continue;

        bool isCamera = false;

        v4l2_input input;
        memset(&input, 0, sizeof(input));
        for (; ::ioctl(fd, VIDIOC_ENUMINPUT, &input) >= 0; ++input.index) {
            if(input.type == V4L2_INPUT_TYPE_CAMERA || input.type == 0) {
                isCamera = ::ioctl(fd, VIDIOC_S_INPUT, input.index) != 0;
                break;
            }
        }

        if (isCamera) {
            // find out its driver "name"
            QString name;
            struct v4l2_capability vcap;
            memset(&vcap, 0, sizeof(struct v4l2_capability));

            if (ioctl(fd, VIDIOC_QUERYCAP, &vcap) != 0)
                name = entryInfo.fileName();
            else
                name = QString((const char*)vcap.card);
            //qDebug() << "found camera: " << name;

            m_names.append(entryInfo.filePath());
            m_descriptions.append(name);
        }
        qt_safe_close(fd);
    }
}

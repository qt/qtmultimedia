/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINRTVIDEODEVICESELECTORCONTROL_H
#define QWINRTVIDEODEVICESELECTORCONTROL_H

#include <QtMultimedia/QVideoDeviceSelectorControl>
#include <QtMultimedia/QCameraInfoControl>
#include <QtCore/qt_windows.h>

struct IInspectable;
namespace ABI {
    namespace Windows {
        namespace Devices {
            namespace Enumeration {
                struct IDeviceInformation;
            }
        }
    }
}

QT_BEGIN_NAMESPACE

class QWinRTVideoDeviceSelectorControlPrivate;
class QWinRTVideoDeviceSelectorControl : public QVideoDeviceSelectorControl
{
    Q_OBJECT
public:
    explicit QWinRTVideoDeviceSelectorControl(QObject *parent = 0);
    ~QWinRTVideoDeviceSelectorControl();

    int deviceCount() const Q_DECL_OVERRIDE;

    QString deviceName(int index) const Q_DECL_OVERRIDE;
    QString deviceDescription(int index) const Q_DECL_OVERRIDE;

    int defaultDevice() const Q_DECL_OVERRIDE;
    int selectedDevice() const Q_DECL_OVERRIDE;

    static QCamera::Position cameraPosition(const QString &deviceName);
    static int cameraOrientation(const QString &deviceName);
    static QList<QByteArray> deviceNames();
    static QByteArray deviceDescription(const QByteArray &deviceName);
    static QByteArray defaultDeviceName();

public slots:
    void setSelectedDevice(int index) Q_DECL_OVERRIDE;

private:
    QScopedPointer<QWinRTVideoDeviceSelectorControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTVideoDeviceSelectorControl)
};

QT_END_NAMESPACE

#endif // QWINRTVIDEODEVICESELECTORCONTROL_H

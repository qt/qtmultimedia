/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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
    explicit QWinRTVideoDeviceSelectorControl(QObject *parent = nullptr);
    ~QWinRTVideoDeviceSelectorControl() override;

    int deviceCount() const override;

    QString deviceName(int index) const override;
    QString deviceDescription(int index) const override;

    int defaultDevice() const override;
    int selectedDevice() const override;

    static QCamera::Position cameraPosition(const QString &deviceName);
    static int cameraOrientation(const QString &deviceName);
    static QList<QByteArray> deviceNames();
    static QByteArray deviceDescription(const QByteArray &deviceName);
    static QByteArray defaultDeviceName();

public slots:
    void setSelectedDevice(int index) override;

private:
    QScopedPointer<QWinRTVideoDeviceSelectorControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTVideoDeviceSelectorControl)
};

QT_END_NAMESPACE

#endif // QWINRTVIDEODEVICESELECTORCONTROL_H

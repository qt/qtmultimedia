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

#ifndef QGSTREAMERVIDEOINPUTDEVICECONTROL_H
#define QGSTREAMERVIDEOINPUTDEVICECONTROL_H

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

#include <private/qgsttools_global_p.h>
#include <qvideodeviceselectorcontrol.h>
#include <QtCore/qstringlist.h>

#include <gst/gst.h>
#include <qcamera.h>

QT_BEGIN_NAMESPACE

class Q_GSTTOOLS_EXPORT QGstreamerVideoInputDeviceControl : public QVideoDeviceSelectorControl
{
Q_OBJECT
public:
    QGstreamerVideoInputDeviceControl(QObject *parent);
    QGstreamerVideoInputDeviceControl(GstElementFactory *factory, QObject *parent);
    ~QGstreamerVideoInputDeviceControl();

    int deviceCount() const override;

    QString deviceName(int index) const override;
    QString deviceDescription(int index) const override;

    int defaultDevice() const override;
    int selectedDevice() const override;

    static QString primaryCamera() { return tr("Main camera"); }
    static QString secondaryCamera() { return tr("Front camera"); }

public Q_SLOTS:
    void setSelectedDevice(int index) override;

private:
    GstElementFactory *m_factory = nullptr;

    int m_selectedDevice = 0;
};

QT_END_NAMESPACE

#endif // QGSTREAMERAUDIOINPUTDEVICECONTROL_H

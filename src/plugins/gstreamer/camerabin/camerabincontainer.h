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


#ifndef CAMERABINMEDIACONTAINERCONTROL_H
#define CAMERABINMEDIACONTAINERCONTROL_H

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <qmediacontainercontrol.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qset.h>

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#if QT_CONFIG(gstreamer_encodingprofiles)
#include <gst/pbutils/encoding-profile.h>
#include <private/qgstcodecsinfo_p.h>
#endif

QT_BEGIN_NAMESPACE

class CameraBinContainer : public QMediaContainerControl
{
Q_OBJECT
public:
    CameraBinContainer(QObject *parent);
    virtual ~CameraBinContainer() {}

    QStringList supportedContainers() const override;
    QString containerDescription(const QString &formatMimeType) const override;

    QString containerFormat() const override;
    void setContainerFormat(const QString &format) override;

    QString actualContainerFormat() const;
    void setActualContainerFormat(const QString &containerFormat);
    void resetActualContainerFormat();

#if QT_CONFIG(gstreamer_encodingprofiles)
    GstEncodingContainerProfile *createProfile();
#endif

Q_SIGNALS:
    void settingsChanged();

private:
    QString m_format;
    QString m_actualFormat;

#if QT_CONFIG(gstreamer_encodingprofiles)
    QGstCodecsInfo m_supportedContainers;
#endif
};

QT_END_NAMESPACE

#endif // CAMERABINMEDIACONTAINERCONTROL_H

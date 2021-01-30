/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QGSTREAMERCONTAINERCONTROL_H
#define QGSTREAMERCONTAINERCONTROL_H

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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <qmediacontainercontrol.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qset.h>

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include <gst/pbutils/encoding-profile.h>
#include <private/qgstcodecsinfo_p.h>

QT_BEGIN_NAMESPACE

class QGStreamerAudioEncoderControl;
class QGStreamerVideoEncoderControl;

class QGStreamerContainerControl : public QMediaContainerControl
{
Q_OBJECT
public:
    QGStreamerContainerControl(QObject *parent);
    virtual ~QGStreamerContainerControl() {}

    QStringList supportedContainers() const override;
    QString containerDescription(const QString &formatMimeType) const override;

    QString containerFormat() const override;
    void setContainerFormat(const QString &format) override;

    QString actualContainerFormat() const;
    void setActualContainerFormat(const QString &containerFormat);
    void resetActualContainerFormat();

    GstEncodingContainerProfile *createProfile();

    GstEncodingContainerProfile *fullProfile(QGStreamerAudioEncoderControl *audioEncoderControl, QGStreamerVideoEncoderControl *videoEncoderControl);

    void applySettings(QGStreamerAudioEncoderControl *audioEncoderControl, QGStreamerVideoEncoderControl *videoEncoderControl);

    QByteArray formatElementName() const { return m_supportedContainers.codecElement(containerFormat()); }
    QSet<QString> supportedStreamTypes(const QString &container) const;

    QString containerExtension() const;

Q_SIGNALS:
    void settingsChanged();

private:
    QString m_format;
    QString m_actualFormat;

    QGstCodecsInfo m_supportedContainers;
};

QT_END_NAMESPACE

#endif // CAMERABINMEDIACONTAINERCONTROL_H

/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKIMAGEENCODERCONTROL_H
#define MOCKIMAGEENCODERCONTROL_H

#include "qimageencodercontrol.h"

class MockImageEncoderControl : public QImageEncoderControl
{
public:
    MockImageEncoderControl(QObject *parent = 0)
        : QImageEncoderControl(parent)
    {
        m_settings = QImageEncoderSettings();
    }

    QList<QSize> supportedResolutions(const QImageEncoderSettings & settings = QImageEncoderSettings(),
                                      bool *continuous = 0) const
    {
        if (continuous)
            *continuous = true;

        QList<QSize> resolutions;
        if (settings.resolution().isValid()) {
            if (settings.resolution() == QSize(160,160) ||
                settings.resolution() == QSize(320,240))
                resolutions << settings.resolution();

            if (settings.quality() == QtMultimedia::HighQuality && settings.resolution() == QSize(640,480))
                resolutions << settings.resolution();
        } else {
            resolutions << QSize(160, 120);
            resolutions << QSize(320, 240);
            if (settings.quality() == QtMultimedia::HighQuality)
                resolutions << QSize(640, 480);
        }

        return resolutions;
    }

    QStringList supportedImageCodecs() const
    {
        QStringList codecs;
        codecs << "PNG" << "JPEG";
        return codecs;
    }

    QString imageCodecDescription(const QString &codecName) const {
        if (codecName == "PNG")
            return QString("Portable Network Graphic");
        if (codecName == "JPEG")
            return QString("Joint Photographic Expert Group");
        return QString();
    }

    QImageEncoderSettings imageSettings() const { return m_settings; }
    void setImageSettings(const QImageEncoderSettings &settings) { m_settings = settings; }

private:
    QImageEncoderSettings m_settings;
};


#endif // MOCKIMAGEENCODERCONTROL_H

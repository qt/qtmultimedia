/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef S60IMAGEENCODERCONTROL_H
#define S60IMAGEENCODERCONTROL_H

#include <QtCore/qobject.h>
#include "qimageencodercontrol.h"

QT_USE_NAMESPACE

class S60ImageCaptureSession;

/*
 * Control for setting encoding settings for the captured image.
 */
class S60ImageEncoderControl : public QImageEncoderControl
{
    Q_OBJECT

public: // Contructors & Destructor

    S60ImageEncoderControl(QObject *parent = 0);
    S60ImageEncoderControl(S60ImageCaptureSession *session, QObject *parent = 0);
    ~S60ImageEncoderControl();

public: // QImageEncoderControl

    // Codec
    QStringList supportedImageCodecs() const;
    QString imageCodecDescription(const QString &codec) const;

    // Resolution
    QList<QSize> supportedResolutions(const QImageEncoderSettings &settings,
                                      bool *continuous = 0) const;

    // Settings
    QImageEncoderSettings imageSettings() const;
    void setImageSettings(const QImageEncoderSettings &settings);

private: // Data

    S60ImageCaptureSession  *m_session;
};

#endif // S60IMAGEENCODERCONTROL_H

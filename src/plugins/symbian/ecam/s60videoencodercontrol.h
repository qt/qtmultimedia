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

#ifndef S60VIDEOENCODERCONTROL_H
#define S60VIDEOENCODERCONTROL_H

#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>

#include "qvideoencodercontrol.h"

QT_USE_NAMESPACE

class S60VideoCaptureSession;

/*
 * Control for video settings when recording video using QMediaRecorder.
 */
class S60VideoEncoderControl : public QVideoEncoderControl
{
    Q_OBJECT

public: // Contructors & Destructor

    S60VideoEncoderControl(QObject *parent = 0);
    S60VideoEncoderControl(S60VideoCaptureSession *session, QObject *parent = 0);
    virtual ~S60VideoEncoderControl();

public: // QVideoEncoderControl

    // Resolution
    QList<QSize> supportedResolutions(const QVideoEncoderSettings &settings, bool *continuous = 0) const;

    // Framerate
    QList<qreal>  supportedFrameRates(const QVideoEncoderSettings &settings, bool *continuous = 0) const;

    // Video Codec
    QStringList supportedVideoCodecs() const;
    QString videoCodecDescription(const QString &codecName) const;

    // Video Settings
    QVideoEncoderSettings videoSettings() const;
    void setVideoSettings(const QVideoEncoderSettings &settings);

    // Encoding Options
    QStringList supportedEncodingOptions(const QString &codec) const;
    QVariant encodingOption(const QString &codec, const QString &name) const;
    void setEncodingOption(const QString &codec, const QString &name, const QVariant &value);

private: // Data

    S60VideoCaptureSession* m_session;

};

#endif // S60VIDEOENCODERCONTROL_H

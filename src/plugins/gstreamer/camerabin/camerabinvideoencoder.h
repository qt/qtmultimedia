/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
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

#ifndef CAMERABINVIDEOENCODE_H
#define CAMERABINVIDEOENCODE_H

#include <qvideoencodercontrol.h>
class CameraBinSession;

#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>

#include <gst/gst.h>

QT_USE_NAMESPACE

class CameraBinVideoEncoder : public QVideoEncoderControl
{
    Q_OBJECT
public:
    CameraBinVideoEncoder(CameraBinSession *session);
    virtual ~CameraBinVideoEncoder();

    QList<QSize> supportedResolutions(const QVideoEncoderSettings &settings = QVideoEncoderSettings(),
                                      bool *continuous = 0) const;

    QList< qreal > supportedFrameRates(const QVideoEncoderSettings &settings = QVideoEncoderSettings(),
                                       bool *continuous = 0) const;

    QPair<int,int> rateAsRational(qreal) const;

    QStringList supportedVideoCodecs() const;
    QString videoCodecDescription(const QString &codecName) const;

    QVideoEncoderSettings videoSettings() const;
    void setVideoSettings(const QVideoEncoderSettings &settings);

    QStringList supportedEncodingOptions(const QString &codec) const;
    QVariant encodingOption(const QString &codec, const QString &name) const;
    void setEncodingOption(const QString &codec, const QString &name, const QVariant &value);

    GstElement *createEncoder();

    QSet<QString> supportedStreamTypes(const QString &codecName) const;

    void setActualVideoSettings(const QVideoEncoderSettings&);
    void resetActualSettings();

Q_SIGNALS:
    void settingsChanged();

private:
    CameraBinSession *m_session;

    QStringList m_codecs;
    QMap<QString,QString> m_codecDescriptions;
    QMap<QString,QByteArray> m_elementNames;
    QMap<QString,QStringList> m_codecOptions;

    QVideoEncoderSettings m_videoSettings; // backend selected settings, using m_userSettings
    QVideoEncoderSettings m_userSettings;

    QMap<QString, QMap<QString, QVariant> > m_options;
    QMap<QString, QSet<QString> > m_streamTypes;
};

#endif

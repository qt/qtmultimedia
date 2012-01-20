/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#ifndef MOCKVIDEOENCODERCONTROL_H
#define MOCKVIDEOENCODERCONTROL_H

#include "qvideoencodercontrol.h"

class MockVideoEncoderControl : public QVideoEncoderControl
{
    Q_OBJECT
public:
    MockVideoEncoderControl(QObject *parent):
        QVideoEncoderControl(parent)
    {
        m_supportedEncodeOptions.insert("video/3gpp", QStringList() << "quantizer" << "me");
        m_supportedEncodeOptions.insert("video/H264", QStringList() << "quantizer" << "me" << "bframes");
        m_videoCodecs << "video/3gpp" << "video/H264";
        m_sizes << QSize(320,240) << QSize(640,480);
        m_framerates << 30 << 15 << 1;
    }
    ~MockVideoEncoderControl() {}

    QVideoEncoderSettings videoSettings() const { return m_videoSettings; }
    void setVideoSettings(const QVideoEncoderSettings &settings) { m_videoSettings = settings; };

    QList<QSize> supportedResolutions(const QVideoEncoderSettings & = QVideoEncoderSettings(),
                                      bool *continuous = 0) const
    {
        if (continuous)
            *continuous = true;

        return m_sizes;
    }

    QList<qreal> supportedFrameRates(const QVideoEncoderSettings & = QVideoEncoderSettings(),
                                     bool *continuous = 0) const
    {
        if (continuous)
            *continuous = false;

        return m_framerates;
    }

    QStringList supportedVideoCodecs() const { return m_videoCodecs; }
    QString videoCodecDescription(const QString &codecName) const { return codecName; }

    QStringList supportedEncodingOptions(const QString &codec) const
    {
        return m_supportedEncodeOptions.value(codec);
    }

    QVariant encodingOption(const QString &codec, const QString &name) const
    {
        return m_encodeOptions[codec].value(name);
    }

    void setEncodingOption(const QString &codec, const QString &name, const QVariant &value)
    {
        m_encodeOptions[codec][name] = value;
    }

private:
    QVideoEncoderSettings m_videoSettings;

    QMap<QString, QStringList> m_supportedEncodeOptions;
    QMap< QString, QMap<QString, QVariant> > m_encodeOptions;

    QStringList m_videoCodecs;
    QList<QSize> m_sizes;
    QList<qreal> m_framerates;
};

#endif // MOCKVIDEOENCODERCONTROL_H

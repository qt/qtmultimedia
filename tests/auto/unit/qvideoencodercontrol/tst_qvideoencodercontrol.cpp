/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include "qvideoencodercontrol.h"
class MyVideEncoderControl: public QVideoEncoderControl
{
    Q_OBJECT

public:
    MyVideEncoderControl(QObject *parent = 0 ):QVideoEncoderControl(parent)
    {

    }

    ~MyVideEncoderControl()
    {

    }

    QList<QSize> supportedResolutions(const QVideoEncoderSettings &settings,bool *continuous = 0) const
    {
        Q_UNUSED(settings);
        Q_UNUSED(continuous);

        return (QList<QSize>());
    }

    QList<qreal> supportedFrameRates(const QVideoEncoderSettings &settings, bool *continuous = 0) const
    {
        Q_UNUSED(settings);
        Q_UNUSED(continuous);

        return (QList<qreal>());

    }

    QStringList supportedVideoCodecs() const
    {
        return QStringList();

    }

    QString videoCodecDescription(const QString &codecName) const
    {
        Q_UNUSED(codecName)
        return QString();

    }

    QVideoEncoderSettings videoSettings() const
    {
        return QVideoEncoderSettings();
    }

    void setVideoSettings(const QVideoEncoderSettings &settings)
    {
        Q_UNUSED(settings);
    }
};

class tst_QVideoEncoderControl: public QObject
{
    Q_OBJECT
private slots:
    void constructor();
};

void tst_QVideoEncoderControl::constructor()
{
    QObject parent;
    MyVideEncoderControl control(&parent);
}

QTEST_MAIN(tst_QVideoEncoderControl)
#include "tst_qvideoencodercontrol.moc"



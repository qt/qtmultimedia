/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#ifndef CAMERABINIMAGEENCODE_H
#define CAMERABINIMAGEENCODE_H

class CameraBinSession;

#include <qimageencodercontrol.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qmap.h>

#include <gst/gst.h>
QT_USE_NAMESPACE

class CameraBinImageEncoder : public QImageEncoderControl
{
    Q_OBJECT
public:
    CameraBinImageEncoder(CameraBinSession *session);
    virtual ~CameraBinImageEncoder();

    QList<QSize> supportedResolutions(const QImageEncoderSettings &settings = QImageEncoderSettings(),
                                      bool *continuous = 0) const;

    QStringList supportedImageCodecs() const;
    QString imageCodecDescription(const QString &formatName) const;

    QImageEncoderSettings imageSettings() const;
    void setImageSettings(const QImageEncoderSettings &settings);

Q_SIGNALS:
    void settingsChanged();

private:
    QImageEncoderSettings m_settings;

    CameraBinSession *m_session;

    // Added
    QStringList m_codecs;
    QMap<QString,QByteArray> m_elementNames;
    QMap<QString,QString> m_codecDescriptions;
    QMap<QString,QStringList> m_codecOptions;
};

#endif

/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QAUDIORECORDER_H
#define QAUDIORECORDER_H

#include <qmediarecorder.h>
#include <qmediaobject.h>
#include <qmediaencodersettings.h>
#include <qmediaenumdebug.h>

#include <QtCore/qpair.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Multimedia)

class QString;
class QSize;
class QAudioFormat;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

class QAudioRecorderPrivate;
class Q_MULTIMEDIA_EXPORT QAudioRecorder : public QMediaRecorder
{
    Q_OBJECT
    Q_PROPERTY(QString audioInput READ audioInput WRITE setAudioInput NOTIFY audioInputChanged)
public:
    QAudioRecorder(QObject *parent = 0, QMediaServiceProvider *serviceProvider = QMediaServiceProvider::defaultServiceProvider());
    ~QAudioRecorder();

    QStringList audioInputs() const;
    QString defaultAudioInput() const;
    QString audioInputDescription(const QString& name) const;

    QString audioInput() const;

public Q_SLOTS:
    void setAudioInput(const QString& name);

Q_SIGNALS:
    void audioInputChanged(const QString& name);
    void availableAudioInputsChanged();

private:
    Q_DISABLE_COPY(QAudioRecorder)
    Q_DECLARE_PRIVATE(QAudioRecorder)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif  // QAUDIORECORDER_H

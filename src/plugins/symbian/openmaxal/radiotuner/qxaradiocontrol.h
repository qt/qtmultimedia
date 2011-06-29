/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QXARADIOCONTROL_H
#define QXARADIOCONTROL_H

#include <QObject>
#include <QRadioTunerControl>

QT_USE_NAMESPACE

class QXARadioSession;

class QXARadioControl : public QRadioTunerControl
{
    Q_OBJECT

public:
    QXARadioControl(QXARadioSession *session, QObject *parent = 0);
    virtual ~QXARadioControl();
    QRadioTuner::State state() const;

    QRadioTuner::Band band() const;
    void setBand(QRadioTuner::Band band);
    bool isBandSupported(QRadioTuner::Band band) const;
    int frequency() const;
    int frequencyStep(QRadioTuner::Band band) const;
    QPair<int,int> frequencyRange(QRadioTuner::Band band) const;
    void setFrequency(int freq);
    bool isStereo() const;
    QRadioTuner::StereoMode stereoMode() const;
    void setStereoMode(QRadioTuner::StereoMode stereoMode);
    int signalStrength() const;
    int volume() const;
    void setVolume(int volume);
    bool isMuted() const;
    void setMuted(bool muted);
    bool isSearching() const;
    void searchForward();
    void searchBackward();
    void cancelSearch();
    bool isValid() const;
    bool isAvailable() const;
    QtMultimediaKit::AvailabilityError availabilityError() const;
    void start();
    void stop();
    QRadioTuner::Error error() const;
    QString errorString() const;

private:
    QXARadioSession *m_session;

protected:
    QXARadioControl(QObject* parent = 0);
};

#endif /* QXARADIOCONTROL_H */

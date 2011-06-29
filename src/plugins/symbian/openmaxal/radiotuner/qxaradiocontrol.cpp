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

#include "qxaradiocontrol.h"
#include "qxaradiosession.h"
#include "xaradiosessionimpl.h"

QXARadioControl::QXARadioControl(QXARadioSession *session, QObject *parent)
:QRadioTunerControl(parent), m_session(session)
{

    connect(m_session, SIGNAL(stateChanged(QRadioTuner::State)), this, SIGNAL(stateChanged(QRadioTuner::State)));

    connect(m_session, SIGNAL(bandChanged(QRadioTuner::Band)), this, SIGNAL(bandChanged(QRadioTuner::Band)));

    connect(m_session, SIGNAL(frequencyChanged(int)), this, SIGNAL(frequencyChanged(int)));

    connect(m_session, SIGNAL(stereoStatusChanged(bool)), this, SIGNAL(stereoStatusChanged(bool)));

    connect(m_session, SIGNAL(searchingChanged(bool)), this, SIGNAL(searchingChanged(bool)));

    connect(m_session, SIGNAL(signalStrengthChanged(int)), this, SIGNAL(signalStrengthChanged(int)));

    connect(m_session, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)));

    connect(m_session, SIGNAL(mutedChanged(bool)), this, SIGNAL(mutedChanged(bool)));

//   connect(m_session, SIGNAL(error(int,QString)), this,SIGNAL(error(int,QString)));
}

QXARadioControl::~QXARadioControl()
{

}

QtMultimediaKit::AvailabilityError QXARadioControl::availabilityError() const
{
    return m_session->availabilityError();
}

bool QXARadioControl::isAvailable() const
{
    return m_session->isAvailable();
}

QRadioTuner::State QXARadioControl::state() const
{
    return m_session->state();
}

QRadioTuner::Band QXARadioControl::band() const
{
    return m_session->band();
}

void QXARadioControl::setBand(QRadioTuner::Band band)
{
    m_session->setBand(band);
}

bool QXARadioControl::isBandSupported(QRadioTuner::Band band) const
{
    return m_session->isBandSupported(band);
}

int QXARadioControl::frequency() const
{
    return m_session->frequency();
}

int QXARadioControl::frequencyStep(QRadioTuner::Band band) const
{
    return m_session->frequencyStep(band);
}

QPair<int,int> QXARadioControl::frequencyRange(QRadioTuner::Band band) const
{
    return m_session->frequencyRange(band);
}

void QXARadioControl::setFrequency(int freq)
{
    m_session->setFrequency(freq);
}

bool QXARadioControl::isStereo() const
{
    return m_session->isStereo();
}
    
QRadioTuner::StereoMode QXARadioControl::stereoMode() const
{
    return m_session->stereoMode();
}           

void QXARadioControl::setStereoMode(QRadioTuner::StereoMode stereoMode)
{    
    m_session->setStereoMode(stereoMode);
}
    
int QXARadioControl::signalStrength() const
{
    return m_session->signalStrength();
}    
    
int QXARadioControl::volume() const
{
    return m_session->volume();
}   
    
void QXARadioControl::setVolume(int volume)
{    
    m_session->setVolume(volume);
}  
    
bool QXARadioControl::isMuted() const
{
    return m_session->isMuted();
}    
    
void QXARadioControl::setMuted(bool muted)
{    
    m_session->setMuted(muted);
}   
    
bool QXARadioControl::isSearching() const
{
    return m_session->isSearching();
}   
    
void QXARadioControl::searchForward() 
{
    m_session->searchForward();
} 
    
void QXARadioControl::searchBackward() 
{
    m_session->searchBackward();
}   
    
void QXARadioControl::cancelSearch() 
{
    m_session->cancelSearch();
}  
    
void QXARadioControl::start() 
{
    m_session->start();
}    
    
void QXARadioControl::stop() 
{
    m_session->stop();
}                              
    
QRadioTuner::Error QXARadioControl::error() const
{
    return m_session->error();
} 
    
QString QXARadioControl::errorString() const
{
    return m_session->errorString();
}        

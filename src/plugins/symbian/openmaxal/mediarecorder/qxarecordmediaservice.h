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

#ifndef QXARECORDMEDIASERVICE_H
#define QXARECORDMEDIASERVICE_H

#include <QtCore/qobject.h>
#include <qmediaservice.h>

QT_USE_NAMESPACE

/*
 * This class implements QMediaService interface.
 */

class QXARecordSession;
class QXAMediaRecoderControl;
class QXAAudioEndpointSelector;
class QXAAudioEncoderControl;
class QXAMediaContainerControl;

class QXARecodMediaService : public QMediaService
{

    Q_OBJECT

public:
    QXARecodMediaService(QObject *parent = 0);
    ~QXARecodMediaService();
    QMediaControl *requestControl(const char *name);
    void releaseControl( QMediaControl *control);
private:
    QXARecordSession *m_session;
    QXAMediaRecoderControl *m_control;
    QXAAudioEndpointSelector *m_endpoint;
    QXAAudioEncoderControl *m_encoder;
    QXAMediaContainerControl *m_container;
};

#endif /* QXARECORDMEDIASERVICE_H */

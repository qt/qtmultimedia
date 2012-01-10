/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef MOCKSERVICE_H
#define MOCKSERVICE_H

#include "qmediaservice.h"

#include "mockaudioencodercontrol.h"
#include "mockmediarecordercontrol.h"
#include "mockvideoencodercontrol.h"
#include "mockaudioendpointselector.h"
#include "mockmediacontainercontrol.h"
#include "mockmetadatawritercontrol.h"

class MockMediaRecorderService : public QMediaService
{
    Q_OBJECT
public:
    MockMediaRecorderService(QObject *parent = 0, QMediaControl *control = 0):
        QMediaService(parent),
        mockControl(control),
        hasControls(true)
    {
        mockAudioEndpointSelector = new MockAudioEndpointSelector(parent);
        mockAudioEncoderControl = new MockAudioEncoderControl(parent);
        mockFormatControl = new MockMediaContainerControl(parent);
        mockVideoEncoderControl = new MockVideoEncoderControl(parent);
        mockMetaDataControl = new MockMetaDataWriterControl(parent);
    }

    QMediaControl* requestControl(const char *name)
    {
        if (hasControls && qstrcmp(name,QAudioEncoderControl_iid) == 0)
            return mockAudioEncoderControl;
        if (hasControls && qstrcmp(name,QAudioEndpointSelector_iid) == 0)
            return mockAudioEndpointSelector;
        if (hasControls && qstrcmp(name,QMediaRecorderControl_iid) == 0)
            return mockControl;
        if (hasControls && qstrcmp(name,QMediaContainerControl_iid) == 0)
            return mockFormatControl;
        if (hasControls && qstrcmp(name,QVideoEncoderControl_iid) == 0)
            return mockVideoEncoderControl;
        if (hasControls && qstrcmp(name, QMetaDataWriterControl_iid) == 0)
            return mockMetaDataControl;

        return 0;
    }

    void releaseControl(QMediaControl*)
    {
    }

    QMediaControl   *mockControl;
    QAudioEndpointSelector  *mockAudioEndpointSelector;
    QAudioEncoderControl    *mockAudioEncoderControl;
    QMediaContainerControl     *mockFormatControl;
    QVideoEncoderControl    *mockVideoEncoderControl;
    MockMetaDataWriterControl *mockMetaDataControl;
    bool hasControls;
};

#endif // MOCKSERVICE_H

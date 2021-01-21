/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
#include "mockmediacontainercontrol.h"
#include "mockmetadatawritercontrol.h"
#include "mockaudioprobecontrol.h"
#include <private/qmediaplatformcaptureinterface_p.h>

class MockMediaRecorderService : public QMediaPlatformCaptureInterface
{
    Q_OBJECT
public:
    MockMediaRecorderService()
        : hasControls(true)
    {
        mockControl = new MockMediaRecorderControl(this);
        mockAudioEncoderControl = new MockAudioEncoderControl(this);
        mockFormatControl = new MockMediaContainerControl(this);
        mockVideoEncoderControl = new MockVideoEncoderControl(this);
        mockMetaDataControl = new MockMetaDataWriterControl(this);
        mockAudioProbeControl = new MockAudioProbeControl(this);
    }

    QObject *requestControl(const char *name)
    {
        if (hasControls && qstrcmp(name,QAudioEncoderSettingsControl_iid) == 0)
            return mockAudioEncoderControl;
        if (hasControls && qstrcmp(name,QMediaRecorderControl_iid) == 0)
            return mockControl;
        if (hasControls && qstrcmp(name,QMediaContainerControl_iid) == 0)
            return mockFormatControl;
        if (hasControls && qstrcmp(name,QVideoEncoderSettingsControl_iid) == 0)
            return mockVideoEncoderControl;
        if (hasControls && qstrcmp(name, QMetaDataWriterControl_iid) == 0)
            return mockMetaDataControl;
        if (hasControls && qstrcmp(name, QMediaAudioProbeControl_iid) == 0)
            return mockAudioProbeControl;

        return 0;
    }

    void releaseControl(QObject *)
    {
    }

    MockMediaRecorderControl *mockControl;
    QAudioEncoderSettingsControl *mockAudioEncoderControl;
    QMediaContainerControl *mockFormatControl;
    QVideoEncoderSettingsControl *mockVideoEncoderControl;
    MockMetaDataWriterControl *mockMetaDataControl;
    MockAudioProbeControl *mockAudioProbeControl;

    bool hasControls;
};

#endif // MOCKSERVICE_H

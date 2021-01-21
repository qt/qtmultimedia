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
#include "mockcamerafocuscontrol.h"
#include "mockcameraimageprocessingcontrol.h"
#include "mockcameraimagecapturecontrol.h"
#include "mockcameraexposurecontrol.h"
#include "mockimageencodercontrol.h"
#include "mockcameracontrol.h"
#include "mockvideorenderercontrol.h"
#include "mockvideowindowcontrol.h"
#include "mockvideodeviceselectorcontrol.h"
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
        mockCameraControl = new MockCameraControl(this);
        mockExposureControl = new MockCameraExposureControl(this);
        mockFocusControl = new MockCameraFocusControl(this);
        mockCaptureControl = new MockCaptureControl(mockCameraControl, this);
        mockImageProcessingControl = new MockImageProcessingControl(this);
        mockImageEncoderControl = new MockImageEncoderControl(this);
        rendererControl = new MockVideoRendererControl(this);
        windowControl = new MockVideoWindowControl(this);
        mockVideoDeviceSelectorControl = new MockVideoDeviceSelectorControl(this);
        rendererRef = 0;
        windowRef = 0;
    }

    QObject *requestControl(const char *name)
    {
        if (!hasControls)
            return nullptr;

        if (qstrcmp(name,QAudioEncoderSettingsControl_iid) == 0)
            return mockAudioEncoderControl;
        if (qstrcmp(name,QMediaRecorderControl_iid) == 0)
            return mockControl;
        if (qstrcmp(name,QMediaContainerControl_iid) == 0)
            return mockFormatControl;
        if (qstrcmp(name,QVideoEncoderSettingsControl_iid) == 0)
            return mockVideoEncoderControl;
        if (qstrcmp(name, QMetaDataWriterControl_iid) == 0)
            return mockMetaDataControl;
        if (qstrcmp(name, QMediaAudioProbeControl_iid) == 0)
            return mockAudioProbeControl;

        if (qstrcmp(name, QCameraControl_iid) == 0)
            return mockCameraControl;

        if (simpleCamera)
            return nullptr;

        if (qstrcmp(name, QCameraExposureControl_iid) == 0)
            return mockExposureControl;
        if (qstrcmp(name, QCameraFocusControl_iid) == 0)
            return mockFocusControl;
        if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
            return mockCaptureControl;
        if (qstrcmp(name, QCameraImageProcessingControl_iid) == 0)
            return mockImageProcessingControl;
        if (qstrcmp(name, QImageEncoderControl_iid) == 0)
            return mockImageEncoderControl;
        if (qstrcmp(name, QVideoDeviceSelectorControl_iid) == 0)
            return mockVideoDeviceSelectorControl;

        if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
            if (rendererRef == 0) {
                rendererRef += 1;
                return rendererControl;
            }
        }
        if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
            if (windowRef == 0) {
                windowRef += 1;
                return windowControl;
            }
        }

        return nullptr;
    }

    void releaseControl(QObject *control)
    {
        if (control == rendererControl)
            rendererRef -= 1;
        if (control == windowControl)
            windowRef -= 1;
    }

    static bool simpleCamera;

    MockCameraControl *mockCameraControl;
    MockCaptureControl *mockCaptureControl;
    MockCameraExposureControl *mockExposureControl;
    MockCameraFocusControl *mockFocusControl;
    MockImageProcessingControl *mockImageProcessingControl;
    MockImageEncoderControl *mockImageEncoderControl;
    MockVideoRendererControl *rendererControl;
    MockVideoWindowControl *windowControl;
    MockVideoDeviceSelectorControl *mockVideoDeviceSelectorControl;
    int rendererRef;
    int windowRef;

    MockMediaRecorderControl *mockControl;
    QAudioEncoderSettingsControl *mockAudioEncoderControl;
    QMediaContainerControl *mockFormatControl;
    QVideoEncoderSettingsControl *mockVideoEncoderControl;
    MockMetaDataWriterControl *mockMetaDataControl;
    MockAudioProbeControl *mockAudioProbeControl;

    bool hasControls;
};

#endif // MOCKSERVICE_H

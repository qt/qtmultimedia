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

#ifndef MOCKCAMERASERVICE_H
#define MOCKCAMERASERVICE_H

#include "qmediaservice.h"
#include "../qmultimedia_common/mockcamerafocuscontrol.h"
#include "../qmultimedia_common/mockcameraimageprocessingcontrol.h"
#include "../qmultimedia_common/mockcameraimagecapturecontrol.h"
#include "../qmultimedia_common/mockcameraexposurecontrol.h"
#include "../qmultimedia_common/mockcameracapturebuffercontrol.h"
#include "../qmultimedia_common/mockimageencodercontrol.h"
#include "../qmultimedia_common/mockcameracontrol.h"
#include "../qmultimedia_common/mockvideosurface.h"
#include "../qmultimedia_common/mockvideorenderercontrol.h"
#include "../qmultimedia_common/mockvideowindowcontrol.h"
#include "../qmultimedia_common/mockvideodeviceselectorcontrol.h"

class MockSimpleCameraService : public QMediaService
{
    Q_OBJECT

public:
    MockSimpleCameraService(): QMediaService(0)
    {
        mockControl = new MockCameraControl(this);
    }

    ~MockSimpleCameraService()
    {
    }

    QObject *requestControl(const char *iid)
    {
        if (qstrcmp(iid, QCameraControl_iid) == 0)
            return mockControl;
        return 0;
    }

    void releaseControl(QObject *) {}

    MockCameraControl *mockControl;
};


class MockCameraService : public QMediaService
{
    Q_OBJECT

public:
    MockCameraService(): QMediaService(0)
    {
        mockControl = new MockCameraControl(this);
        mockExposureControl = new MockCameraExposureControl(this);
        mockFocusControl = new MockCameraFocusControl(this);
        mockCaptureControl = new MockCaptureControl(mockControl, this);
        mockCaptureBufferControl = new MockCaptureBufferFormatControl(this);
        mockImageProcessingControl = new MockImageProcessingControl(this);
        mockImageEncoderControl = new MockImageEncoderControl(this);
        rendererControl = new MockVideoRendererControl(this);
        windowControl = new MockVideoWindowControl(this);
        mockVideoDeviceSelectorControl = new MockVideoDeviceSelectorControl(this);
        rendererRef = 0;
        windowRef = 0;
    }

    ~MockCameraService()
    {
    }

    QObject *requestControl(const char *iid)
    {
        if (qstrcmp(iid, QCameraControl_iid) == 0)
            return mockControl;

        if (qstrcmp(iid, QCameraExposureControl_iid) == 0)
            return mockExposureControl;

        if (qstrcmp(iid, QCameraFocusControl_iid) == 0)
            return mockFocusControl;

        if (qstrcmp(iid, QCameraImageCaptureControl_iid) == 0)
            return mockCaptureControl;

        if (qstrcmp(iid, QCameraCaptureBufferFormatControl_iid) == 0)
            return mockCaptureBufferControl;

        if (qstrcmp(iid, QCameraImageProcessingControl_iid) == 0)
            return mockImageProcessingControl;

        if (qstrcmp(iid, QImageEncoderControl_iid) == 0)
            return mockImageEncoderControl;

        if (qstrcmp(iid, QVideoDeviceSelectorControl_iid) == 0)
            return mockVideoDeviceSelectorControl;

        if (qstrcmp(iid, QVideoRendererControl_iid) == 0) {
            if (rendererRef == 0) {
                rendererRef += 1;
                return rendererControl;
            }
        }
        if (qstrcmp(iid, QVideoWindowControl_iid) == 0) {
            if (windowRef == 0) {
                windowRef += 1;
                return windowControl;
            }
        }

        return 0;
    }

    void releaseControl(QObject *control)
    {
        if (control == rendererControl)
            rendererRef -= 1;
        if (control == windowControl)
            windowRef -= 1;
    }

    MockCameraControl *mockControl;
    MockCaptureControl *mockCaptureControl;
    MockCaptureBufferFormatControl *mockCaptureBufferControl;
    MockCameraExposureControl *mockExposureControl;
    MockCameraFocusControl *mockFocusControl;
    MockImageProcessingControl *mockImageProcessingControl;
    MockImageEncoderControl *mockImageEncoderControl;
    MockVideoRendererControl *rendererControl;
    MockVideoWindowControl *windowControl;
    MockVideoDeviceSelectorControl *mockVideoDeviceSelectorControl;
    int rendererRef;
    int windowRef;
};

#endif // MOCKCAMERASERVICE_H

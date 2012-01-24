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

#ifndef MOCKCAMERASERVICE_H
#define MOCKCAMERASERVICE_H

#include "qmediaservice.h"
#include "../qmultimedia_common/mockcameraflashcontrol.h"
#include "../qmultimedia_common/mockcameralockscontrol.h"
#include "../qmultimedia_common/mockcamerafocuscontrol.h"
#include "../qmultimedia_common/mockcamerazoomcontrol.h"
#include "../qmultimedia_common/mockcameraimageprocessingcontrol.h"
#include "../qmultimedia_common/mockcameraimagecapturecontrol.h"
#include "../qmultimedia_common/mockcameraexposurecontrol.h"
#include "../qmultimedia_common/mockcameracapturedestinationcontrol.h"
#include "../qmultimedia_common/mockcameracapturebuffercontrol.h"
#include "../qmultimedia_common/mockimageencodercontrol.h"
#include "../qmultimedia_common/mockcameracontrol.h"
#include "../qmultimedia_common/mockvideosurface.h"
#include "../qmultimedia_common/mockvideorenderercontrol.h"

#if defined(QT_MULTIMEDIA_MOCK_WIDGETS)
#include "../qmultimedia_common/mockvideowindowcontrol.h"
#endif

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

    QMediaControl* requestControl(const char *iid)
    {
        if (qstrcmp(iid, QCameraControl_iid) == 0)
            return mockControl;
        return 0;
    }

    void releaseControl(QMediaControl*) {}

    MockCameraControl *mockControl;
};


class MockCameraService : public QMediaService
{
    Q_OBJECT

public:
    MockCameraService(): QMediaService(0)
    {
        mockControl = new MockCameraControl(this);
        mockLocksControl = new MockCameraLocksControl(this);
        mockExposureControl = new MockCameraExposureControl(this);
        mockFlashControl = new MockCameraFlashControl(this);
        mockFocusControl = new MockCameraFocusControl(this);
        mockZoomControl = new MockCameraZoomControl(this);
        mockCaptureControl = new MockCaptureControl(mockControl, this);
        mockCaptureBufferControl = new MockCaptureBufferFormatControl(this);
        mockCaptureDestinationControl = new MockCaptureDestinationControl(this);
        mockImageProcessingControl = new MockImageProcessingControl(this);
        mockImageEncoderControl = new MockImageEncoderControl(this);
        rendererControl = new MockVideoRendererControl(this);
#if defined(QT_MULTIMEDIA_MOCK_WIDGETS)
        windowControl = new MockVideoWindowControl(this);
#endif
        rendererRef = 0;
        windowRef = 0;
    }

    ~MockCameraService()
    {
    }

    QMediaControl* requestControl(const char *iid)
    {
        if (qstrcmp(iid, QCameraControl_iid) == 0)
            return mockControl;

        if (qstrcmp(iid, QCameraLocksControl_iid) == 0)
            return mockLocksControl;

        if (qstrcmp(iid, QCameraExposureControl_iid) == 0)
            return mockExposureControl;

        if (qstrcmp(iid, QCameraFlashControl_iid) == 0)
            return mockFlashControl;

        if (qstrcmp(iid, QCameraFocusControl_iid) == 0)
            return mockFocusControl;

        if (qstrcmp(iid, QCameraZoomControl_iid) == 0)
            return mockZoomControl;

        if (qstrcmp(iid, QCameraImageCaptureControl_iid) == 0)
            return mockCaptureControl;

        if (qstrcmp(iid, QCameraCaptureBufferFormatControl_iid) == 0)
            return mockCaptureBufferControl;

        if (qstrcmp(iid, QCameraCaptureDestinationControl_iid) == 0)
            return mockCaptureDestinationControl;

        if (qstrcmp(iid, QCameraImageProcessingControl_iid) == 0)
            return mockImageProcessingControl;

        if (qstrcmp(iid, QImageEncoderControl_iid) == 0)
            return mockImageEncoderControl;

        if (qstrcmp(iid, QVideoRendererControl_iid) == 0) {
            if (rendererRef == 0) {
                rendererRef += 1;
                return rendererControl;
            }
        }
#if defined(QT_MULTIMEDIA_MOCK_WIDGETS)
        if (qstrcmp(iid, QVideoWindowControl_iid) == 0) {
            if (windowRef == 0) {
                windowRef += 1;
                return windowControl;
            }
        }
#endif
        return 0;
    }

    void releaseControl(QMediaControl *control)
    {
        if (control == rendererControl)
            rendererRef -= 1;
#if defined(QT_MULTIMEDIA_MOCK_WIDGETS)
        if (control == windowControl)
            windowRef -= 1;
#endif
    }

    MockCameraControl *mockControl;
    MockCameraLocksControl *mockLocksControl;
    MockCaptureControl *mockCaptureControl;
    MockCaptureBufferFormatControl *mockCaptureBufferControl;
    MockCaptureDestinationControl *mockCaptureDestinationControl;
    MockCameraExposureControl *mockExposureControl;
    MockCameraFlashControl *mockFlashControl;
    MockCameraFocusControl *mockFocusControl;
    MockCameraZoomControl *mockZoomControl;
    MockImageProcessingControl *mockImageProcessingControl;
    MockImageEncoderControl *mockImageEncoderControl;
    MockVideoRendererControl *rendererControl;
#if defined(QT_MULTIMEDIA_MOCK_WIDGETS)
    MockVideoWindowControl *windowControl;
#endif
    int rendererRef;
    int windowRef;
};

#endif // MOCKCAMERASERVICE_H

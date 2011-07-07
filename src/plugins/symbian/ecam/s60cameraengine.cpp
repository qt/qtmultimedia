/****************************************************************************
 **
 ** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
 ** All rights reserved.
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file is part of the Qt Mobility Components.
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

#include "s60cameraengine.h"
#include "s60cameraengineobserver.h"
#include "s60cameraconstants.h"
#include <QtCore/qglobal.h>
#include <fbs.h> // CFbsBitmap
#ifdef ECAM_PREVIEW_API
    #include <platform/ecam/camerasnapshot.h>
#endif // ECAM_PREVIEW_API

CCameraEngine::CCameraEngine()
{
}

CCameraEngine::CCameraEngine(TInt aCameraHandle,
                             TInt aPriority,
                             MCameraEngineObserver* aObserver) :
    // CBase initializes member variables to NULL
    iObserver(aObserver),
    iCameraIndex(aCameraHandle),
    iPriority(aPriority),
    iEngineState(EEngineNotReady),
    iCaptureResolution(TSize(0,0)),
    iNew2LImplementation(false),
    iLatestImageBufferIndex(1) // Thus we start from index 0
{
    // Observer is mandatory
    ASSERT(aObserver != NULL);
}

CCameraEngine::~CCameraEngine()
{
    StopViewFinder();
    ReleaseViewFinderBuffer();  // Releases iViewFinderBuffer
    ReleaseImageBuffer();       // Releases iImageBuffer + iImageBitmap

    iAdvancedSettingsObserver = NULL;
    iImageCaptureObserver = NULL;
    iViewfinderObserver = NULL;

#ifdef S60_CAM_AUTOFOCUS_SUPPORT
    delete iAutoFocus;
#endif // S60_CAM_AUTOFOCUS_SUPPORT

    if (iCamera) {
        iCamera->Release();
        delete iCamera;
        iCamera = NULL;
    }
}

TInt CCameraEngine::CamerasAvailable()
{
    return CCamera::CamerasAvailable();
}

CCameraEngine* CCameraEngine::NewL(TInt aCameraHandle,
                                   TInt aPriority,
                                   MCameraEngineObserver* aObserver)
{
    CCameraEngine* self = new (ELeave) CCameraEngine(aCameraHandle, aPriority, aObserver);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
}

void CCameraEngine::ConstructL()
{
    if (!CCamera::CamerasAvailable())
        User::Leave(KErrHardwareNotAvailable);

#ifndef Q_CC_NOKIAX86 // Not Emulator
    TInt err(KErrNone);
#else // Emulator
    TInt err(KErrNotFound);
#endif // !(Q_CC_NOKIAX86)

#ifdef S60_31_PLATFORM
    // Construct CCamera object for S60 3.1 (NewL)
    iNew2LImplementation = false;
    TRAP(err, iCamera = CCamera::NewL(*this, iCameraIndex));
    if (err)
        User::Leave(err);
#else // For S60 3.2 onwards - use this constructor (New2L)
    iNew2LImplementation = true;
    TRAP(err, iCamera = CCamera::New2L(*this, iCameraIndex, iPriority));
    if (err)
        User::Leave(err);
#endif // S60_31_PLATFORM

#ifdef S60_CAM_AUTOFOCUS_SUPPORT
    // Might not be supported for secondary camera, discard errors
    TRAP(err, iAutoFocus = CCamAutoFocus::NewL(iCamera));
#endif // S60_CAM_AUTOFOCUS_SUPPORT

    if (iCamera == NULL)
        User::Leave(KErrNoMemory);

    iCamera->CameraInfo(iCameraInfo);
}

void CCameraEngine::SetAdvancedObserver(MAdvancedSettingsObserver* aAdvancedSettingsObserver)
{
    iAdvancedSettingsObserver = aAdvancedSettingsObserver;
}

void CCameraEngine::SetImageCaptureObserver(MCameraEngineImageCaptureObserver* aImageCaptureObserver)
{
    iImageCaptureObserver = aImageCaptureObserver;
}

void CCameraEngine::SetViewfinderObserver(MCameraViewfinderObserver* aViewfinderObserver)
{
    iViewfinderObserver = aViewfinderObserver;
}

void CCameraEngine::ReserveAndPowerOn()
{
    if (!iCamera || iEngineState > EEngineNotReady) {
        iObserver->MceoHandleError(EErrReserve, KErrNotReady);
        return;
    }

    iCamera->Reserve();
}

void CCameraEngine::ReleaseAndPowerOff()
{
    if (iEngineState >= EEngineIdle) {
        CancelCapture();
        StopViewFinder();
        FocusCancel();
        iCamera->PowerOff();
        iCamera->Release();
    }
    iEngineState = EEngineNotReady;
}

void CCameraEngine::StartViewFinderL(TSize& aSize)
{
    if (iEngineState < EEngineIdle)
        User::Leave(KErrNotReady);

    if (0 == (iCameraInfo.iOptionsSupported & TCameraInfo::EViewFinderBitmapsSupported))
        User::Leave(KErrNotSupported);

    if (!iCamera->ViewFinderActive()) {
        if (iCameraIndex != 0)
            iCamera->SetViewFinderMirrorL(true);
        iCamera->StartViewFinderBitmapsL(aSize);
    }
}

void CCameraEngine::StopViewFinder()
{
    if (iCamera && iCamera->ViewFinderActive())
        iCamera->StopViewFinder();
}

void CCameraEngine::StartDirectViewFinderL(RWsSession& aSession,
                            CWsScreenDevice& aScreenDevice,
                            RWindowBase& aWindow,
                            TRect& aScreenRect,
                            TRect& aClipRect)
{
    if (iEngineState < EEngineIdle)
        User::Leave(KErrNotReady);

    if (0 == (iCameraInfo.iOptionsSupported & TCameraInfo::EViewFinderDirectSupported))
        User::Leave(KErrNotSupported);

    if (!iCamera->ViewFinderActive()) {
        // Viewfinder extent needs to be clipped according to the clip rect.
        // This is because the native camera framework does not support
        // clipping and starting viewfinder with bigger than the display(S60
        // 5.0 and older)/window(Symbian^3 and later) would cause viewfinder
        // starting to fail entirely. This causes shrinking effect in some
        // cases, but is better than not having the viewfinder at all.
        if (aScreenRect.Intersects(aClipRect))
            aScreenRect.Intersection(aClipRect);

        if (iCameraIndex != 0)
            iCamera->SetViewFinderMirrorL(true);
        if (aScreenRect.Width() > 0 && aScreenRect.Height() > 0) {
            iCamera->StartViewFinderDirectL(aSession, aScreenDevice, aWindow, aScreenRect);
        } else {
            if (iObserver)
                iObserver->MceoHandleError(EErrViewFinderReady, KErrArgument);
        }
    }
}

void CCameraEngine::PrepareL(TSize& aCaptureSize, CCamera::TFormat aFormat)
{
    iImageCaptureFormat = aFormat;

    TInt closestVar = KMaxTInt, selected = 0;
    TSize size;

    // Scan through supported capture sizes and select the closest match
    for (TInt index = 0; index < iCameraInfo.iNumImageSizesSupported; index++) {

        iCamera->EnumerateCaptureSizes(size, index, aFormat);
        if (size == aCaptureSize) {
            selected = index;
            break;
        }

        TSize varSz = size - aCaptureSize;
        TInt variation = varSz.iWidth * varSz.iHeight;
        if (variation < closestVar) {
            closestVar = variation;
            selected = index;
        }
    }

    iCamera->EnumerateCaptureSizes(aCaptureSize, selected, aFormat);
    iCaptureResolution = aCaptureSize;
    iCamera->PrepareImageCaptureL(aFormat, selected);
}

void CCameraEngine::CaptureL()
{
    if (iEngineState < EEngineIdle)
        User::Leave(KErrNotReady);

    iCamera->CaptureImage();
    iEngineState = EEngineCapturing;
}

void CCameraEngine::CancelCapture()
{
    if (iEngineState == EEngineCapturing) {
        iCamera->CancelCaptureImage();
        iEngineState = EEngineIdle;
    }
}

void CCameraEngine::HandleEvent(const TECAMEvent &aEvent)
{
    if (aEvent.iEventType == KUidECamEventReserveComplete) {
        ReserveComplete(aEvent.iErrorCode);
        return;
    }

    if (aEvent.iEventType == KUidECamEventPowerOnComplete) {
        PowerOnComplete(aEvent.iErrorCode);
        return;
    }

    if (aEvent.iEventType == KUidECamEventCameraNoLongerReserved) {
        // All camera related operations need to be stopped
        iObserver->MceoHandleError(EErrReserve, KErrHardwareNotAvailable);
        return;
    }

#ifdef ECAM_PREVIEW_API
    if (aEvent.iEventType == KUidECamEventCameraSnapshot) {
        HandlePreview();
        return;
    }
#endif // ECAM_PREVIEW_API

#if !defined(Q_CC_NOKIAX86) // Not Emulator
    // Other events; Exposure, Zoom, etc. (See ecamadvancedsettings.h)
    if (iAdvancedSettingsObserver)
        iAdvancedSettingsObserver->HandleAdvancedEvent(aEvent);

    if (iImageCaptureObserver)
        iImageCaptureObserver->MceoHandleOtherEvent(aEvent);
#endif // !Q_CC_NOKIAX86
}

void CCameraEngine::ReserveComplete(TInt aError)
{
    if (aError == KErrNone) {
        iCamera->PowerOn();
#ifdef S60_31_PLATFORM
    } else if (aError == KErrAlreadyExists) { // Known Issue on some S60 3.1 devices
        User::After(500000); // Wait for 0,5 second and try again
        iCamera->Reserve();
#endif // S60_31_PLATFORM
    } else {
        iObserver->MceoHandleError(EErrReserve, aError);
    }
}

void CCameraEngine::PowerOnComplete(TInt aError)
{
    if (aError) {
        iObserver->MceoHandleError(EErrPowerOn, aError);
        iEngineState = EEngineNotReady;
        return;
    }

    // Init AutoFocus
#ifndef Q_CC_NOKIAX86  // Not Emulator
#ifdef S60_CAM_AUTOFOCUS_SUPPORT // S60 3.1
    if( iAutoFocus ) {
        TRAPD(afErr, iAutoFocus->InitL( *this ));
        if (afErr) {
            delete iAutoFocus;
            iAutoFocus = 0;
        }
    }
#endif // S60_CAM_AUTOFOCUS_SUPPORT
#endif // !Q_CC_NOKIAX86

    iEngineState = EEngineIdle;
    iObserver->MceoCameraReady();
}

#ifdef ECAM_PREVIEW_API
/**
 * This method creates the CCameraPreview object and requests the previews to
 * be provided during the image or video capture
 */
void CCameraEngine::EnablePreviewProvider(MCameraPreviewObserver *aPreviewObserver)
{
    // Delete old one if exists
    if (iCameraSnapshot)
        delete iCameraSnapshot;

    iPreviewObserver = aPreviewObserver;

    TInt error = KErrNone;

    if (iCamera) {
        TRAP(error, iCameraSnapshot = CCamera::CCameraSnapshot::NewL(*iCamera));
        if (error) {
            if (iObserver)
                iObserver->MceoHandleError(EErrPreview, error);
            return;
        }

        TRAP(error, iCameraSnapshot->PrepareSnapshotL(KDefaultFormatPreview, SelectPreviewResolution(), EFalse));
        if (error) {
            if (iObserver)
                iObserver->MceoHandleError(EErrPreview, error);
            return;
        }

        iCameraSnapshot->StartSnapshot();
    } else {
        if (iObserver)
            iObserver->MceoHandleError(EErrPreview, KErrNotReady);
    }
}

/**
 * This method disables and destroys the CCameraPreview object. Thus previews
 * will not be provided during the image or video capture.
 */
void CCameraEngine::DisablePreviewProvider()
{
    if (!iCameraSnapshot)
        return;

    iCameraSnapshot->StopSnapshot();

    delete iCameraSnapshot;
    iCameraSnapshot = 0;

    iPreviewObserver = 0;
}
#endif // ECAM_PREVIEW_API

/*
 * MCameraObserver2:
 * New viewfinder frame available
 */
void CCameraEngine::ViewFinderReady(MCameraBuffer &aCameraBuffer, TInt aError)
{
    iViewFinderBuffer = &aCameraBuffer;

    if (aError == KErrNone) {
        if (iViewfinderObserver) {
            TRAPD(err, iViewfinderObserver->MceoViewFinderFrameReady(aCameraBuffer.BitmapL(0)));
            if (err)
                iObserver->MceoHandleError(EErrViewFinderReady, err);
        } else {
            iObserver->MceoHandleError(EErrViewFinderReady, KErrNotReady);
        }
    }
    else {
        iObserver->MceoHandleError(EErrViewFinderReady, aError);
    }
}

/*
 * MCameraObserver:
 * New viewfinder frame available
 */
void CCameraEngine::ViewFinderFrameReady(CFbsBitmap& aFrame)
{
    if (iViewfinderObserver)
        iViewfinderObserver->MceoViewFinderFrameReady(aFrame);
    else
        iObserver->MceoHandleError(EErrViewFinderReady, KErrNotReady);
}

void CCameraEngine::ReleaseViewFinderBuffer()
{
    if (iNew2LImplementation) { // NewL Implementation does not use MCameraBuffer
        if (iViewFinderBuffer) {
            iViewFinderBuffer->Release();
            iViewFinderBuffer = NULL;
        }
    }
}

void CCameraEngine::ReleaseImageBuffer()
{
    // Reset Bitmap
    if (iLatestImageBufferIndex == 1 || iImageBitmap2 == NULL) {
        if (iImageBitmap1) {
            if (!iNew2LImplementation) { // NewL - Ownership transferred
                iImageBitmap1->Reset(); // Reset/Delete Bitmap
                delete iImageBitmap1;
            }
            iImageBitmap1 = NULL;
        }
    } else {
        if (iImageBitmap2) {
            if (!iNew2LImplementation) { // NewL - Ownership transferred
                iImageBitmap2->Reset(); // Reset/Delete Bitmap
                delete iImageBitmap2;
            }
            iImageBitmap2 = NULL;
        }
    }

    // Reset Data pointers
    if (iLatestImageBufferIndex == 1 || iImageData2 == NULL) {
        if (!iNew2LImplementation) // NewL - Ownership transfers with buffer
            delete iImageData1;
        iImageData1 = NULL;
    } else {
        if (!iNew2LImplementation) // NewL - Ownership transfers with buffer
            delete iImageData2;
        iImageData2 = NULL;
    }

    // Reset ImageBuffer - New2L Implementation only
    if (iLatestImageBufferIndex == 1 || iImageBuffer2 == NULL) {
        if (iImageBuffer1) {
            iImageBuffer1->Release();
            iImageBuffer1 = NULL;
        }
    } else {
        if (iImageBuffer2) {
            iImageBuffer2->Release();
            iImageBuffer2 = NULL;
        }
    }
}

/*
 * MCameraObserver2
 * Captured image is ready (New2L version)
 */
void CCameraEngine::ImageBufferReady(MCameraBuffer &aCameraBuffer, TInt aError)
{
    // Use the buffer that is available
    if (!iImageBuffer1) {
        iLatestImageBufferIndex = 0;
        iImageBuffer1 = &aCameraBuffer;
    } else {
        iLatestImageBufferIndex = 1;
        iImageBuffer2 = &aCameraBuffer;
    }

    bool isBitmap = true;
    TInt err = KErrNone;

    switch (iImageCaptureFormat) {
        case CCamera::EFormatFbsBitmapColor4K:
        case CCamera::EFormatFbsBitmapColor64K:
        case CCamera::EFormatFbsBitmapColor16M:
        case CCamera::EFormatFbsBitmapColor16MU:
            if (iLatestImageBufferIndex == 0) {
                TRAP(err, iImageBitmap1 = &iImageBuffer1->BitmapL(0));
                if (err) {
                    if (iImageCaptureObserver)
                        iImageCaptureObserver->MceoHandleError(EErrImageReady, err);
                }
            } else {
                TRAP(err, iImageBitmap2 = &iImageBuffer2->BitmapL(0));
                if (err) {
                    if (iImageCaptureObserver)
                        iImageCaptureObserver->MceoHandleError(EErrImageReady, err);
                }
            }
            isBitmap = true;
            break;
        case CCamera::EFormatExif:
            if (iLatestImageBufferIndex == 0) {
                TRAP(err, iImageData1 = iImageBuffer1->DataL(0));
                if (err) {
                    if (iImageCaptureObserver)
                        iImageCaptureObserver->MceoHandleError(EErrImageReady, err);
                }
            } else {
                TRAP(err, iImageData2 = iImageBuffer2->DataL(0));
                if (err) {
                    if (iImageCaptureObserver)
                        iImageCaptureObserver->MceoHandleError(EErrImageReady, err);
                }
            }
            isBitmap = false;
            break;

        default:
            if (iImageCaptureObserver)
                iImageCaptureObserver->MceoHandleError(EErrImageReady, KErrNotSupported);
            return;
    }

    // Handle captured image
    HandleImageReady(aError, isBitmap);
}

/*
 * MCameraObserver
 * Captured image is ready (NewL version)
 */
void CCameraEngine::ImageReady(CFbsBitmap* aBitmap, HBufC8* aData, TInt aError)
{
    bool isBitmap = true;

    // Toggle between the 2 buffers
    if (iLatestImageBufferIndex == 1) {
        iLatestImageBufferIndex = 0;
    } else {
        iLatestImageBufferIndex = 1;
    }

    switch (iImageCaptureFormat) {
        case CCamera::EFormatFbsBitmapColor4K:
        case CCamera::EFormatFbsBitmapColor64K:
        case CCamera::EFormatFbsBitmapColor16M:
        case CCamera::EFormatFbsBitmapColor16MU:
            if (iLatestImageBufferIndex == 0)
                iImageBitmap1 = aBitmap;
            else
                iImageBitmap2 = aBitmap;
            isBitmap = true;
            break;
        case CCamera::EFormatExif:
            if (iLatestImageBufferIndex == 0)
                iImageData1 = aData;
            else
                iImageData2 = aData;
            isBitmap = false;
            break;

        default:
            if (iImageCaptureObserver)
                iImageCaptureObserver->MceoHandleError(EErrImageReady, KErrNotSupported);
            return;
    }

    // Handle captured image
    HandleImageReady(aError, isBitmap);
}

void CCameraEngine::HandleImageReady(const TInt aError, const bool isBitmap)
{
    iEngineState = EEngineIdle;

    if (aError == KErrNone) {
        if (isBitmap)
            if (iImageCaptureObserver) {
                if (iLatestImageBufferIndex == 0)
                    iImageCaptureObserver->MceoCapturedBitmapReady(iImageBitmap1);
                else
                    iImageCaptureObserver->MceoCapturedBitmapReady(iImageBitmap2);
            }
            else
                ReleaseImageBuffer();
        else {
            if (iImageCaptureObserver) {
                if (iLatestImageBufferIndex == 0)
                    iImageCaptureObserver->MceoCapturedDataReady(iImageData1);
                else
                    iImageCaptureObserver->MceoCapturedDataReady(iImageData2);
            }
            else
                ReleaseImageBuffer();
        }
    } else {
        if (iImageCaptureObserver)
            iImageCaptureObserver->MceoHandleError(EErrImageReady, aError);
    }
}

#ifdef ECAM_PREVIEW_API
void CCameraEngine::HandlePreview()
{
    if (!iCameraSnapshot) {
        if (iObserver)
            iObserver->MceoHandleError(EErrPreview, KErrGeneral);
        return;
    }

    RArray<TInt> previewIndices;
    CleanupClosePushL(previewIndices);

    MCameraBuffer &newPreview = iCameraSnapshot->SnapshotDataL(previewIndices);

    for (TInt i = 0; i < previewIndices.Count(); ++i)
        iPreviewObserver->MceoPreviewReady(newPreview.BitmapL(0));

    CleanupStack::PopAndDestroy(); // RArray<TInt> previewIndices
}

TSize CCameraEngine::SelectPreviewResolution()
{
    TSize currentResolution(iCaptureResolution);

    TSize previewResolution(0, 0);
    if (currentResolution == TSize(4000,2248) ||
        currentResolution == TSize(3264,1832) ||
        currentResolution == TSize(2592,1456) ||
        currentResolution == TSize(1920,1080) ||
        currentResolution == TSize(1280,720)) {
        previewResolution = KDefaultSizePreview_Wide;
    } else if (currentResolution == TSize(352,288) ||
        currentResolution == TSize(176,144)) {
        previewResolution = KDefaultSizePreview_CIF;
    } else if (currentResolution == TSize(720,576)) {
        previewResolution = KDefaultSizePreview_PAL;
    } else if (currentResolution == TSize(720,480)) {
        previewResolution = KDefaultSizePreview_NTSC;
    } else {
        previewResolution = KDefaultSizePreview_Normal;
    }

    return previewResolution;
}
#endif // ECAM_PREVIEW_API

//=============================================================================
// S60 3.1 - AutoFocus support (Other platforms, see S60CameraSettings class)
//=============================================================================

void CCameraEngine::InitComplete(TInt aError)
{
    if (aError) {
        if (iImageCaptureObserver)
            iImageCaptureObserver->MceoHandleError(EErrAutoFocusInit, aError);
    }
}

void CCameraEngine::OptimisedFocusComplete(TInt aError)
{
    iEngineState = EEngineIdle;

    if (aError == KErrNone)
        if (iImageCaptureObserver)
            iImageCaptureObserver->MceoFocusComplete();
    else {
        if (iImageCaptureObserver)
            iImageCaptureObserver->MceoHandleError(EErrOptimisedFocusComplete, aError);
    }
}

TBool CCameraEngine::IsCameraReady() const
{
    // If reserved and powered on, but not focusing or capturing
    if (iEngineState == EEngineIdle)
        return ETrue;

    return EFalse;
}

TBool CCameraEngine::IsDirectViewFinderSupported() const
{
    if (iCameraInfo.iOptionsSupported & TCameraInfo::EViewFinderDirectSupported)
        return true;
    else
        return false;
}

TCameraInfo *CCameraEngine::CameraInfo()
{
    return &iCameraInfo;
}

TBool CCameraEngine::IsAutoFocusSupported() const
{
#ifndef Q_CC_NOKIAX86 // Not Emulator

#ifdef S60_CAM_AUTOFOCUS_SUPPORT // S60 3.1
    return (iAutoFocus) ? ETrue : EFalse;
#else // !S60_CAM_AUTOFOCUS_SUPPORT
    return EFalse;
#endif // S60_CAM_AUTOFOCUS_SUPPORT

#else // Q_CC_NOKIAX86 - Emulator
    return EFalse;
#endif // !Q_CC_NOKIAX86
}

/*
 * This function is used for focusing in S60 3.1 platform. Platforms from S60
 * 3.2 onwards should use the focusing provided by the S60CameraSettings class.
 */
void CCameraEngine::StartFocusL()
{
    if (iEngineState != EEngineIdle)
        return;

#ifndef Q_CC_NOKIAX86  // Not Emulator
#ifdef S60_CAM_AUTOFOCUS_SUPPORT // S60 3.1
    if (iAutoFocus) {
        if (!iAFRange) {
            iAFRange = CCamAutoFocus::ERangeNormal;
            iAutoFocus->SetFocusRangeL(iAFRange);
        }

    iAutoFocus->AttemptOptimisedFocusL();
    iEngineState = EEngineFocusing;
    }
#endif // S60_CAM_AUTOFOCUS_SUPPORT
#endif // !Q_CC_NOKIAX86
}

/*
 * This function is used for cancelling focusing in S60 3.1 platform. Platforms
 * from S60 3.2 onwards should use the focusing provided by the
 * S60CameraSettings class.
 */
void CCameraEngine::FocusCancel()
{
#ifndef Q_CC_NOKIAX86 // Not Emulator
#ifdef S60_CAM_AUTOFOCUS_SUPPORT
    if (iAutoFocus) {
        iAutoFocus->Cancel();
        iEngineState = EEngineIdle;
    }
#endif // S60_CAM_AUTOFOCUS_SUPPORT
#endif // !Q_CC_NOKIAX86
}

void CCameraEngine::SupportedFocusRanges(TInt& aSupportedRanges) const
{
    aSupportedRanges = 0;

#ifndef Q_CC_NOKIAX86 // Not Emulator
#ifdef S60_CAM_AUTOFOCUS_SUPPORT
    if (iAutoFocus) {
        // CCamAutoFocus doesn't provide a method for getting supported ranges!
        // Assume everything is supported (rather optimistic)
        aSupportedRanges = CCamAutoFocus::ERangeMacro |
                           CCamAutoFocus::ERangePortrait |
                           CCamAutoFocus::ERangeNormal |
                           CCamAutoFocus::ERangeInfinite;
    }
#endif // S60_CAM_AUTOFOCUS_SUPPORT
#endif // !Q_CC_NOKIAX86
}

void CCameraEngine::SetFocusRange(TInt aFocusRange)
{
#if !defined(Q_CC_NOKIAX86) // Not Emulator

#ifdef S60_CAM_AUTOFOCUS_SUPPORT
    if (iAutoFocus) {
        TRAPD(focusErr, iAutoFocus->SetFocusRangeL((CCamAutoFocus::TAutoFocusRange)aFocusRange));
        if (focusErr)
            iObserver->MceoHandleError(EErrAutoFocusRange, focusErr);
    }
#endif // S60_CAM_AUTOFOCUS_SUPPORT

#else // Q_CC_NOKIAX86 // Emulator
    Q_UNUSED(aFocusRange);
    if (iImageCaptureObserver)
        iImageCaptureObserver->MceoHandleError(EErrAutoFocusRange, KErrNotSupported);
#endif // !Q_CC_NOKIAX86
}

// End of file

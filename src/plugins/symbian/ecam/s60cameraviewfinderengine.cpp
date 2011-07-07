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

#include <QApplication>
#include <QDesktopWidget>
#include <qcamera.h>
#include <qabstractvideosurface.h>
#include <qvideoframe.h>

#include "s60cameraviewfinderengine.h"
#include "s60cameraengine.h"
#include "s60cameracontrol.h"
#include "s60videowidgetcontrol.h"
#include "s60videowidgetdisplay.h"
#include "s60videorenderercontrol.h"
#include "s60videowindowcontrol.h"
#include "s60videowindowdisplay.h"
#include "s60cameraconstants.h"

#include <coemain.h>    // CCoeEnv
#include <coecntrl.h>   // CCoeControl
#include <w32std.h>

// Helper function
TRect qRect2TRect(const QRect &qr)
{
    return TRect(TPoint(qr.left(), qr.top()), TSize(qr.width(), qr.height()));
}


S60CameraViewfinderEngine::S60CameraViewfinderEngine(S60CameraControl *control,
                                                     CCameraEngine *engine,
                                                     QObject *parent):
    QObject(parent),
    m_cameraEngine(engine),
    m_cameraControl(0),
    m_viewfinderOutput(0),
    m_viewfinderDisplay(0),
    m_viewfinderSurface(0),
    m_wsSession(CCoeEnv::Static()->WsSession()),
    m_screenDevice(*CCoeEnv::Static()->ScreenDevice()),
    m_window(0),
    m_desktopWidget(0),
    m_vfState(EVFNotConnectedNotStarted),
    m_viewfinderSize(KDefaultViewfinderSize),
    m_actualViewFinderSize(KDefaultViewfinderSize),
    m_viewfinderAspectRatio(0.0),
    m_viewfinderType(OutputTypeNotSet),
    m_viewfinderNativeType(EBitmapViewFinder), // Default type
    m_isViewFinderVisible(true), // True by default (only QVideoWidgetControl supports being hidden)
    m_uiLandscape(true),
    m_vfErrorsSignalled(0)
{
    m_cameraControl = control;

    // Check whether platform supports DirectScreen ViewFinder
    if (m_cameraEngine) {
        if (m_cameraEngine->IsDirectViewFinderSupported())
            m_viewfinderNativeType = EDirectScreenViewFinder;
        else
            m_viewfinderNativeType = EBitmapViewFinder;

        MCameraViewfinderObserver *vfObserver = this;
        m_cameraEngine->SetViewfinderObserver(vfObserver);
    }
    else
        m_cameraControl->setError(KErrGeneral, tr("Unexpected camera error."));
    // From now on it is safe to assume engine exists

    // Check the UI orientation
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect screenRect = desktopWidget->screenGeometry();
    if (screenRect.width() > screenRect.height())
        m_uiLandscape = true;
    else
        m_uiLandscape = false;

    // Detect UI Rotations
    m_desktopWidget = QApplication::desktop();
    if (m_desktopWidget)
        connect(m_desktopWidget, SIGNAL(resized(int)), this, SLOT(handleDesktopResize(int)));
}

S60CameraViewfinderEngine::~S60CameraViewfinderEngine()
{
    // No need to stop viewfinder:
    // Engine has stopped it already
    // Surface will be stopped by VideoRendererControl

    m_viewfinderOutput = 0;
    m_viewfinderSurface = 0;
}

void S60CameraViewfinderEngine::setNewCameraEngine(CCameraEngine *engine)
{
    m_cameraEngine = engine;

    if (m_cameraEngine) {
        // And set observer to the new CameraEngine
        MCameraViewfinderObserver *vfObserver = this;
        m_cameraEngine->SetViewfinderObserver(vfObserver);
    }
}

void S60CameraViewfinderEngine::handleDesktopResize(int screen)
{
    Q_UNUSED(screen);
    // UI Rotation is handled by the QVideoWidgetControl, thus this is needed
    // only for the QVideoRendererControl
    if (m_viewfinderType == OutputTypeRenderer) {
        QSize newResolution(-1,-1);
        if (m_viewfinderSurface)
            newResolution = m_viewfinderSurface->nativeResolution();

        if (newResolution.width() == -1 || newResolution.height() == -1) {
            QDesktopWidget* desktopWidget = QApplication::desktop();
            QRect screenRect = desktopWidget->screenGeometry();
            newResolution = QSize(screenRect.width(), screenRect.height());
        }

        resetViewfinderSize(newResolution);
    }

    // Rotate Camera if UI has rotated
    checkAndRotateCamera();
}

void S60CameraViewfinderEngine::setVideoWidgetControl(QObject *viewfinderOutput)
{
    // Release old control if it has not already been done
    if (m_viewfinderOutput)
        releaseControl(m_viewfinderType);

    // Rotate Camera if UI has rotated
    checkAndRotateCamera();

    S60VideoWidgetControl* viewFinderWidgetControl =
        qobject_cast<S60VideoWidgetControl*>(viewfinderOutput);

    if (viewFinderWidgetControl) {
        // Check whether platform supports DirectScreen ViewFinder
        if (m_cameraEngine) {
            if (m_cameraEngine->IsDirectViewFinderSupported())
                m_viewfinderNativeType = EDirectScreenViewFinder;
            else
                m_viewfinderNativeType = EBitmapViewFinder;
        }
        else
            return;

        m_viewfinderDisplay = viewFinderWidgetControl->display();

        if (m_viewfinderNativeType == EDirectScreenViewFinder) {
            m_viewfinderDisplay->setPaintingEnabled(false); // No Qt Painter painting - Direct rendering
            connect(m_viewfinderDisplay, SIGNAL(windowHandleChanged(RWindow *)), this, SLOT(resetViewfinderDisplay()));
        } else {
            m_viewfinderDisplay->setPaintingEnabled(true); // Qt Painter painting - Bitmap rendering
            connect(this, SIGNAL(viewFinderFrameReady(const CFbsBitmap &)), m_viewfinderDisplay, SLOT(setFrame(const CFbsBitmap &)));
        }

        connect(m_viewfinderDisplay, SIGNAL(visibilityChanged(bool)), this, SLOT(handleVisibilityChange(bool)));
        connect(m_viewfinderDisplay, SIGNAL(displayRectChanged(QRect, QRect)), this, SLOT(resetVideoWindowSize()));
        connect(m_viewfinderDisplay, SIGNAL(windowHandleChanged(RWindow*)), this, SLOT(handleWindowChange(RWindow*)));

        m_viewfinderSize = m_viewfinderDisplay->extentRect().size();
        m_viewfinderOutput = viewfinderOutput;
        m_viewfinderType = OutputTypeVideoWidget;
        m_isViewFinderVisible = m_viewfinderDisplay->isVisible();

        switch (m_vfState) {
            case EVFNotConnectedNotStarted:
                m_vfState = EVFIsConnectedNotStarted;
                break;
            case EVFNotConnectedIsStarted:
                if (m_isViewFinderVisible)
                    m_vfState = EVFIsConnectedIsStartedIsVisible;
                else
                    m_vfState = EVFIsConnectedIsStartedNotVisible;
                break;
            case EVFIsConnectedNotStarted:
            case EVFIsConnectedIsStartedNotVisible:
            case EVFIsConnectedIsStartedIsVisible:
                // Already connected, state does not change
                break;
            default:
                emit error(QCamera::CameraError, tr("General viewfinder error."));
                break;
        }

        if (m_vfState == EVFIsConnectedIsStartedIsVisible)
            startViewfinder(true); // Internal start (i.e. start if started externally)
    }
}

void S60CameraViewfinderEngine::setVideoRendererControl(QObject *viewfinderOutput)
{
    // Release old control if it has not already been done
    if (m_viewfinderOutput)
        releaseControl(m_viewfinderType);

    // Rotate Camera if UI has rotated
    checkAndRotateCamera();

    S60VideoRendererControl* viewFinderRenderControl =
        qobject_cast<S60VideoRendererControl*>(viewfinderOutput);

    if (viewFinderRenderControl) {
        m_viewfinderNativeType = EBitmapViewFinder; // Always Bitmap

        connect(viewFinderRenderControl, SIGNAL(viewFinderSurfaceSet()),
            this, SLOT(rendererSurfaceSet()));

        Q_ASSERT(!viewFinderRenderControl->surface());
        m_viewfinderOutput = viewfinderOutput;
        m_viewfinderType = OutputTypeRenderer;
        // RendererControl viewfinder is "visible" when surface is set
        m_isViewFinderVisible = false;
        if (EVFIsConnectedIsStartedIsVisible)
            m_vfState = EVFIsConnectedIsStartedNotVisible;

        // Use display resolution as default viewfinder resolution
        m_viewfinderSize = QApplication::desktop()->screenGeometry().size();

        switch (m_vfState) {
        case EVFNotConnectedNotStarted:
            m_vfState = EVFIsConnectedNotStarted;
            break;
        case EVFNotConnectedIsStarted:
            m_vfState = EVFIsConnectedIsStartedIsVisible; // GraphicsItem "always visible" (FrameWork decides to draw/not draw)
            break;
        case EVFIsConnectedNotStarted:
        case EVFIsConnectedIsStartedNotVisible:
        case EVFIsConnectedIsStartedIsVisible:
            // Already connected, state does not change
            break;
        default:
            emit error(QCamera::CameraError, tr("General viewfinder error."));
            break;
        }

        if (m_vfState == EVFIsConnectedIsStartedIsVisible)
            startViewfinder(true);
    }
}

void S60CameraViewfinderEngine::setVideoWindowControl(QObject *viewfinderOutput)
{
    // Release old control if it has not already been done
    if (m_viewfinderOutput)
        releaseControl(m_viewfinderType);

    // Rotate Camera if UI has rotated
    checkAndRotateCamera();

    S60VideoWindowControl* viewFinderWindowControl =
        qobject_cast<S60VideoWindowControl*>(viewfinderOutput);

    if (viewFinderWindowControl) {
        // Check whether platform supports DirectScreen ViewFinder
        if (m_cameraEngine) {
            if (m_cameraEngine->IsDirectViewFinderSupported())
                m_viewfinderNativeType = EDirectScreenViewFinder;
            else
                m_viewfinderNativeType = EBitmapViewFinder;
        } else {
            return;
        }

        m_viewfinderDisplay = viewFinderWindowControl->display();

        if (m_viewfinderNativeType == EDirectScreenViewFinder) {
            m_viewfinderDisplay->setPaintingEnabled(false); // No Qt Painter painting - Direct rendering
            connect(m_viewfinderDisplay, SIGNAL(windowHandleChanged(RWindow *)), this, SLOT(resetViewfinderDisplay()));
        } else {
            m_viewfinderDisplay->setPaintingEnabled(true); // Qt Painter painting - Bitmap rendering
            connect(this, SIGNAL(viewFinderFrameReady(const CFbsBitmap &)), m_viewfinderDisplay, SLOT(setFrame(const CFbsBitmap &)));
        }

        connect(m_viewfinderDisplay, SIGNAL(displayRectChanged(QRect, QRect)), this, SLOT(resetVideoWindowSize()));
        connect(m_viewfinderDisplay, SIGNAL(visibilityChanged(bool)), this, SLOT(handleVisibilityChange(bool)));
        connect(m_viewfinderDisplay, SIGNAL(windowHandleChanged(RWindow*)), this, SLOT(handleWindowChange(RWindow*)));

        m_viewfinderSize = m_viewfinderDisplay->extentRect().size();
        m_viewfinderOutput = viewfinderOutput;
        m_viewfinderType = OutputTypeVideoWindow;
        m_isViewFinderVisible = m_viewfinderDisplay->isVisible();

        switch (m_vfState) {
        case EVFNotConnectedNotStarted:
            m_vfState = EVFIsConnectedNotStarted;
            break;
        case EVFNotConnectedIsStarted:
            if (m_isViewFinderVisible)
                m_vfState = EVFIsConnectedIsStartedIsVisible;
            else
                m_vfState = EVFIsConnectedIsStartedNotVisible;
            break;
        case EVFIsConnectedNotStarted:
        case EVFIsConnectedIsStartedNotVisible:
        case EVFIsConnectedIsStartedIsVisible:
            // Already connected, state does not change
            break;
        default:
            emit error(QCamera::CameraError, tr("General viewfinder error."));
            break;
        }

        if (m_vfState == EVFIsConnectedIsStartedIsVisible)
            startViewfinder(true); // Internal start (i.e. start if started externally)
    }
}

void S60CameraViewfinderEngine::releaseControl(ViewfinderOutputType type)
{
    if (m_vfState == EVFIsConnectedIsStartedIsVisible)
        stopViewfinder(true);

    if (m_viewfinderOutput) {
        switch (type) {
        case OutputTypeNotSet:
            return;
        case OutputTypeVideoWidget:
            if (m_viewfinderType != OutputTypeVideoWidget)
                return;
            disconnect(m_viewfinderOutput);
            m_viewfinderOutput->disconnect(this);
            Q_ASSERT(m_viewfinderDisplay);
            disconnect(m_viewfinderDisplay);
            m_viewfinderDisplay->disconnect(this);
            m_viewfinderDisplay = 0;
            // Invalidate the extent rect
            qobject_cast<S60VideoWidgetControl*>(m_viewfinderOutput)->setExtentRect(QRect());
            break;
        case OutputTypeVideoWindow:
            if (m_viewfinderType != OutputTypeVideoWindow)
                return;
            disconnect(m_viewfinderOutput);
            m_viewfinderOutput->disconnect(this);
            Q_ASSERT(m_viewfinderDisplay);
            disconnect(m_viewfinderDisplay);
            m_viewfinderDisplay->disconnect(this);
            m_viewfinderDisplay = 0;
            break;
        case OutputTypeRenderer:
            if (m_viewfinderType != OutputTypeRenderer)
                return;
            disconnect(m_viewfinderOutput);
            m_viewfinderOutput->disconnect(this);
            if (m_viewfinderSurface)
                m_viewfinderSurface->disconnect(this);
            disconnect(this, SIGNAL(viewFinderFrameReady(const CFbsBitmap &)),
                this, SLOT(viewFinderBitmapReady(const CFbsBitmap &)));
            break;
        default:
            emit error(QCamera::CameraError, tr("Unexpected viewfinder error."));
            return;
        }
    }

    Q_ASSERT(!m_viewfinderDisplay);
    m_viewfinderOutput = 0;
    m_viewfinderType = OutputTypeNotSet;

    // Update state
    switch (m_vfState) {
    case EVFNotConnectedNotStarted:
    case EVFNotConnectedIsStarted:
        // Do nothing
        break;
    case EVFIsConnectedNotStarted:
        m_vfState = EVFNotConnectedNotStarted;
        break;
    case EVFIsConnectedIsStartedNotVisible:
    case EVFIsConnectedIsStartedIsVisible:
        m_vfState = EVFNotConnectedIsStarted;
        break;
    default:
        emit error(QCamera::CameraError, tr("General viewfinder error."));
        break;
    }
}

void S60CameraViewfinderEngine::startViewfinder(const bool internalStart)
{
    if (!internalStart) {
        switch (m_vfState) {
            case EVFNotConnectedNotStarted:
                m_vfState = EVFNotConnectedIsStarted;
                break;
            case EVFIsConnectedNotStarted:
                if (m_isViewFinderVisible)
                    m_vfState = EVFIsConnectedIsStartedIsVisible;
                else
                    m_vfState = EVFIsConnectedIsStartedNotVisible;
                break;
            case EVFNotConnectedIsStarted:
            case EVFIsConnectedIsStartedNotVisible:
            case EVFIsConnectedIsStartedIsVisible:
                // Already started, state does not change
                break;
            default:
                emit error(QCamera::CameraError, tr("General viewfinder error."));
                break;
        }
    }

    // Start viewfinder
    if (m_vfState == EVFIsConnectedIsStartedIsVisible) {

        if (!m_cameraEngine)
            return;

        if (m_viewfinderNativeType == EDirectScreenViewFinder) {

            if (RWindow *window = m_viewfinderDisplay ? m_viewfinderDisplay->windowHandle() : 0) {
                m_window = window;
            } else {
                emit error(QCamera::CameraError, tr("Requesting window for viewfinder failed."));
                return;
            }

            const QRect extentRect = m_viewfinderDisplay ? m_viewfinderDisplay->extentRect() : QRect();
            const QRect clipRect = m_viewfinderDisplay ? m_viewfinderDisplay->clipRect() : QRect();

            TRect extentRectSymbian = qRect2TRect(extentRect);
            TRect clipRectSymbian = qRect2TRect(clipRect);
            TRAPD(err, m_cameraEngine->StartDirectViewFinderL(m_wsSession, m_screenDevice, *m_window, extentRectSymbian, clipRectSymbian));
            if (err) {
                if (err == KErrNotSupported) {
                    emit error(QCamera::NotSupportedFeatureError, tr("Requested viewfinder size is not supported."));
                } else {
                    emit error(QCamera::CameraError, tr("Starting viewfinder failed."));
                }
                return;
            }

            m_actualViewFinderSize = QSize(extentRectSymbian.Size().iWidth, extentRectSymbian.Size().iHeight);
            m_viewfinderAspectRatio = qreal(m_actualViewFinderSize.width()) / qreal(m_actualViewFinderSize.height());

        } else { // Bitmap ViewFinder
            TSize size = TSize(m_viewfinderSize.width(), m_viewfinderSize.height());

            if( m_viewfinderType == OutputTypeRenderer && m_viewfinderSurface) {
                if (!m_surfaceFormat.isValid()) {
                    emit error(QCamera::NotSupportedFeatureError, tr("Invalid surface format."));
                    return;
                }

                // Start rendering to surface with correct size and format
                if (!m_viewfinderSurface->isFormatSupported(m_surfaceFormat) ||
                    !m_viewfinderSurface->start(m_surfaceFormat)) {
                    emit error(QCamera::NotSupportedFeatureError, tr("Failed to start surface."));
                    return;
                }

                if (!m_viewfinderSurface->isActive())
                    return;
            }

            TRAPD(vfErr, m_cameraEngine->StartViewFinderL(size));
            if (vfErr) {
                if (vfErr == KErrNotSupported) {
                    emit error(QCamera::NotSupportedFeatureError, tr("Requested viewfinder size is not supported."));
                } else {
                    emit error(QCamera::CameraError, tr("Starting viewfinder failed."));
                }
                return;
            }

            m_actualViewFinderSize = QSize(size.iWidth, size.iHeight);
            m_viewfinderAspectRatio = qreal(m_actualViewFinderSize.width()) / qreal(m_actualViewFinderSize.height());

            // Notify control about the frame size (triggers frame position calculation)
            if (m_viewfinderDisplay) {
                m_viewfinderDisplay->setNativeSize(m_actualViewFinderSize);
            } else {
                if (m_viewfinderType == OutputTypeRenderer && m_viewfinderSurface) {
                    m_viewfinderSurface->stop();
                    QVideoSurfaceFormat format = m_viewfinderSurface->surfaceFormat();
                    format.setFrameSize(QSize(m_actualViewFinderSize));
                    format.setViewport(QRect(0, 0, m_actualViewFinderSize.width(), m_actualViewFinderSize.height()));
                    m_viewfinderSurface->start(format);
                }
            }
        }
    }
}

void S60CameraViewfinderEngine::stopViewfinder(const bool internalStop)
{
    // Stop if viewfinder is started
    if (m_vfState == EVFIsConnectedIsStartedIsVisible) {
        if (m_viewfinderOutput && m_viewfinderType == OutputTypeRenderer && m_viewfinderSurface) {
            // Stop surface if one still exists
            m_viewfinderSurface->stop();
        }

        if (m_cameraEngine)
            m_cameraEngine->StopViewFinder();
    }

    // Update state
    if (!internalStop) {
        switch (m_vfState) {
            case EVFNotConnectedNotStarted:
            case EVFIsConnectedNotStarted:
                // Discard
                break;
            case EVFNotConnectedIsStarted:
                m_vfState = EVFNotConnectedNotStarted;
                break;
            case EVFIsConnectedIsStartedNotVisible:
            case EVFIsConnectedIsStartedIsVisible:
                m_vfState = EVFIsConnectedNotStarted;
                break;
            default:
                emit error(QCamera::CameraError, tr("General viewfinder error."));
                break;
        }
    }
}

void S60CameraViewfinderEngine::MceoViewFinderFrameReady(CFbsBitmap& aFrame)
{
    emit viewFinderFrameReady(aFrame);
    if (m_cameraEngine)
        m_cameraEngine->ReleaseViewFinderBuffer();
}

void S60CameraViewfinderEngine::resetViewfinderSize(const QSize size)
{
    m_viewfinderSize = size;

    if(m_vfState != EVFIsConnectedIsStartedIsVisible) {
        // Set native size to Window/Renderer Control
        if (m_viewfinderDisplay)
            m_viewfinderDisplay->setNativeSize(m_actualViewFinderSize);
        return;
    }

    stopViewfinder(true);

    startViewfinder(true);
}

void S60CameraViewfinderEngine::resetVideoWindowSize()
{
    if (m_viewfinderDisplay)
        resetViewfinderSize(m_viewfinderDisplay->extentRect().size());
}

void S60CameraViewfinderEngine::resetViewfinderDisplay()
{
    if (m_viewfinderNativeType == EDirectScreenViewFinder) {

        switch (m_viewfinderType) {
        case OutputTypeVideoWidget: {
            if (!m_viewfinderOutput)
                return;

            // First stop viewfinder
            stopViewfinder(true);

            RWindow *window = m_viewfinderDisplay->windowHandle();
            if (!window) {
                return;
            }

            // Then start it with the new WindowID
            startViewfinder(true);
            break;
        }
        case OutputTypeRenderer:
        case OutputTypeVideoWindow:
            // Do nothing
            break;

        default:
            // Not ViewFinder Output has been set, Discard
            break;
        }
    }
}

void S60CameraViewfinderEngine::rendererSurfaceSet()
{
    S60VideoRendererControl* viewFinderRenderControl =
        qobject_cast<S60VideoRendererControl*>(m_viewfinderOutput);

    // Reset old surface if needed
    if (m_viewfinderSurface) {
        handleVisibilityChange(false);
        disconnect(m_viewfinderSurface);
        if (viewFinderRenderControl->surface())
            stopViewfinder(true); // Temporary stop
        else
            stopViewfinder(); // Stop for good
        m_viewfinderSize = QApplication::desktop()->screenGeometry().size();
        m_viewfinderSurface = 0;
    }

    // Set new surface
    m_viewfinderSurface = viewFinderRenderControl->surface();
    if (!m_viewfinderSurface)
        return;
    if (!m_viewfinderSurface->nativeResolution().isEmpty()) {
        if (m_viewfinderSurface->nativeResolution() != m_viewfinderSize)
            resetViewfinderSize(m_viewfinderSurface->nativeResolution());
    }

    connect(m_viewfinderSurface, SIGNAL(nativeResolutionChanged(const QSize&)),
        this, SLOT(resetViewfinderSize(QSize)));

    // Set Surface Properties
    if (m_viewfinderSurface->supportedPixelFormats().contains(QVideoFrame::Format_RGB32))
        m_surfaceFormat = QVideoSurfaceFormat(m_actualViewFinderSize, QVideoFrame::Format_RGB32);
    else if (m_viewfinderSurface->supportedPixelFormats().contains(QVideoFrame::Format_ARGB32))
        m_surfaceFormat = QVideoSurfaceFormat(m_actualViewFinderSize, QVideoFrame::Format_ARGB32);
    else {
        return;
    }
    m_surfaceFormat.setFrameRate(KViewfinderFrameRate);
    m_surfaceFormat.setYCbCrColorSpace(QVideoSurfaceFormat::YCbCr_Undefined); // EColor16MU (compatible with EColor16MA)
    m_surfaceFormat.setPixelAspectRatio(1,1); // PAR 1:1


    connect(this, SIGNAL(viewFinderFrameReady(const CFbsBitmap &)),
        this, SLOT(viewFinderBitmapReady(const CFbsBitmap &)));

    // Surface set, viewfinder is "visible"
    handleVisibilityChange(true);
}

void S60CameraViewfinderEngine::viewFinderBitmapReady(const CFbsBitmap &bitmap)
{
    CFbsBitmap *bitmapPtr = const_cast<CFbsBitmap*>(&bitmap);
    QPixmap pixmap = QPixmap::fromSymbianCFbsBitmap(bitmapPtr);

    QImage newImage = pixmap.toImage();
    if (newImage.format() != QImage::Format_ARGB32 &&
        newImage.format() != QImage::Format_RGB32) {
        newImage = newImage.convertToFormat(QImage::Format_RGB32);
    }

    if (!newImage.isNull()) {
        QVideoFrame newFrame(newImage);
        if (newFrame.isValid()) {
            if (!m_viewfinderSurface->present(newFrame)) {
                // Presenting may fail even if there are no errors (e.g. busy)
                if (m_viewfinderSurface->error()) {
                    if (m_vfErrorsSignalled < KMaxVFErrorsSignalled) {
                        emit error(QCamera::CameraError, tr("Presenting viewfinder frame failed."));
                        ++m_vfErrorsSignalled;
                    }
                }
            }
        } else {
            if (m_vfErrorsSignalled < KMaxVFErrorsSignalled) {
                emit error(QCamera::CameraError, tr("Invalid viewfinder frame was received."));
                ++m_vfErrorsSignalled;
            }
        }

    } else {
        if (m_vfErrorsSignalled < KMaxVFErrorsSignalled) {
            emit error(QCamera::CameraError, tr("Failed to convert viewfinder frame to presentable image."));
            ++m_vfErrorsSignalled;
        }
    }
}

void S60CameraViewfinderEngine::handleVisibilityChange(const bool isVisible)
{
    if (m_isViewFinderVisible == isVisible)
        return;

    m_isViewFinderVisible = isVisible;

    if (m_isViewFinderVisible) {
        switch (m_vfState) {
            case EVFNotConnectedNotStarted:
            case EVFIsConnectedNotStarted:
            case EVFNotConnectedIsStarted:
            case EVFIsConnectedIsStartedIsVisible:
                // Discard
                break;
            case EVFIsConnectedIsStartedNotVisible:
                m_vfState = EVFIsConnectedIsStartedIsVisible;
                break;
            default:
                emit error(QCamera::CameraError, tr("General viewfinder error."));
                break;
        }
        startViewfinder(true);
    } else {
        // Stopping takes care of the state change
        stopViewfinder(true);
    }
}

void S60CameraViewfinderEngine::handleWindowChange(RWindow *handle)
{
    stopViewfinder(true);

    if (handle) // New handle available, start viewfinder
        startViewfinder(true);
}

void S60CameraViewfinderEngine::checkAndRotateCamera()
{
    bool isUiNowLandscape = false;
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect screenRect = desktopWidget->screenGeometry();

    if (screenRect.width() > screenRect.height())
        isUiNowLandscape = true;
    else
        isUiNowLandscape = false;

    // Rotate camera if possible
    if (isUiNowLandscape != m_uiLandscape) {
        stopViewfinder(true);

        // Request orientation reset
        m_cameraControl->resetCameraOrientation();
    }
    m_uiLandscape = isUiNowLandscape;
}

void S60CameraViewfinderEngine::handleContentAspectRatioChange(const QSize& newSize)
{
    qreal newAspectRatio = qreal(newSize.width()) / qreal(newSize.height());
    // Check if aspect ratio changed
    if (qFuzzyCompare(newAspectRatio, m_viewfinderAspectRatio))
        return;

    // Resize viewfinder by reducing either width or height to comply with the new aspect ratio
    QSize newNativeResolution;
    if (newAspectRatio > m_viewfinderAspectRatio) { // New AspectRatio is wider => Reduce height
        newNativeResolution = QSize(m_actualViewFinderSize.width(), (m_actualViewFinderSize.width() / newAspectRatio));
    } else { // New AspectRatio is higher => Reduce width
        newNativeResolution = QSize((m_actualViewFinderSize.height() * newAspectRatio), m_actualViewFinderSize.height());
    }

    // Notify aspect ratio change (use actual content size to notify that)
    // This triggers item size/position re-calculation
    if (m_viewfinderDisplay)
        m_viewfinderDisplay->setNativeSize(newNativeResolution);
}

// End of file

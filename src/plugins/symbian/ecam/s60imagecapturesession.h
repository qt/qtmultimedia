/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef S60IMAGECAPTURESESSION_H
#define S60IMAGECAPTURESESSION_H

#include <QtCore/qurl.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtGui/qicon.h>

#include <qcamera.h>
#include <qcamerafocus.h>
#include <qcameraimagecapture.h>
#include <qvideoframe.h>

#include "s60camerasettings.h"
#include "s60cameraengine.h"
#include "s60cameraengineobserver.h"
#include "s60cameraconstants.h" // Default Jpeg Quality

#include <icl/imagedata.h> // TFrameInfo

QT_USE_NAMESPACE

class S60CameraService;
class CImageDecoder;
class CImageEncoder;
class CFrameImageData;
class RFs;
class S60ImageCaptureSession;

/*
 * This class implements asynchronous image decoding service for the
 * S60ImageCaptureSession.
 */
class S60ImageCaptureDecoder : public CActive
{
public: // Static Contructor & Destructor

    static S60ImageCaptureDecoder* FileNewL(S60ImageCaptureSession *imageSession = 0,
                                        RFs *fileSystemAccess = 0,
                                        const TDesC16 *fileName = 0);
    static S60ImageCaptureDecoder* DataNewL(S60ImageCaptureSession *imageSession = 0,
                                        RFs *fileSystemAccess = 0,
                                        const TDesC8 *data = 0);
    ~S60ImageCaptureDecoder();

public: // Operations

    void decode(CFbsBitmap *destBitmap);
    TFrameInfo *frameInfo();

protected: // CActive

    void RunL();
    void DoCancel();
    TInt RunError(TInt aError);

protected: // Protected constructors

    S60ImageCaptureDecoder(S60ImageCaptureSession *imageSession,
                           RFs *fileSystemAccess,
                           const TDesC8 *data,
                           const TDesC16 *fileName);
    void ConstructL(const bool fileInput = false);

private: // Data

    S60ImageCaptureSession  *m_imageSession;
    CImageDecoder           *m_imageDecoder;
    RFs                     *m_fs;
    const TDesC8            *m_jpegImageData;
    const TDesC16           *m_jpegImageFile;
    bool                    m_fileInput;
    TFrameInfo              m_frameInfo;

};

//=============================================================================

/*
 * This class implements asynchronous image encoding service for the
 * S60ImageCaptureSession.
 */
class S60ImageCaptureEncoder : public CActive
{
public: // Static Contructor & Destructor

    static S60ImageCaptureEncoder* NewL(S60ImageCaptureSession *imageSession = 0,
                                        RFs *fileSystemAccess = 0,
                                        const TDesC16 *fileName = 0,
                                        TInt jpegQuality = KDefaultImageQuality);
    ~S60ImageCaptureEncoder();

public: // Operations

    void encode(CFbsBitmap *sourceBitmap);

protected: // CActive

    void RunL();
    void DoCancel();
    TInt RunError(TInt aError);

protected: // Protected constructors

    S60ImageCaptureEncoder(S60ImageCaptureSession *imageSession,
                           RFs *fileSystemAccess,
                           const TDesC16 *fileName,
                           TInt jpegQuality);
    void ConstructL();

private: // Data

    S60ImageCaptureSession  *m_imageSession;
    CImageEncoder           *m_imageEncoder;
    RFs                     *m_fileSystemAccess;
    const TDesC16           *m_fileName;
    CFrameImageData         *m_frameImageData;
    TInt                    m_jpegQuality;

};

//=============================================================================

/*
 * Session handling all image capture activities.
 */
class S60ImageCaptureSession : public QObject,
#ifdef ECAM_PREVIEW_API
                               public MCameraPreviewObserver,
#endif // ECAM_PREVIEW_API
                               public MCameraEngineImageCaptureObserver
{
    Q_OBJECT

public: // Enums

    enum ImageCaptureState {
        EImageCaptureNotPrepared = 0,   // 0 - ImageCapture has not been prepared
        EImageCapturePrepared,          // 1 - ImageCapture has been prepared
        EImageCaptureCapturing,         // 2 - Image capture ongoing
        EImageCaptureWritingImage       // 3 - Image captured and image writing to file ongoing
    };

public: // Constructor & Destructor

    S60ImageCaptureSession(QObject *parent = 0);
    ~S60ImageCaptureSession();

public: // Methods

    void setError(const TInt error, const QString &description, const bool captureError = false);
    int currentImageId() const;

    bool isDeviceReady();
    void setCameraHandle(CCameraEngine* camerahandle);
    void setCurrentDevice(TInt deviceindex);
    void notifySettingsSet();

    // Ecam Advanced Settings
    S60CameraSettings* advancedSettings();
    void deleteAdvancedSettings();

    // Controls
    int prepareImageCapture();
    void releaseImageCapture();
    int capture(const QString &fileName);
    void cancelCapture();
    void releaseImageBuffer();

    // Image Resolution
    QSize captureSize() const;
    QSize minimumCaptureSize();
    QSize maximumCaptureSize();
    QList<QSize> supportedCaptureSizesForCodec(const QString &codecName);
    void setCaptureSize(const QSize &size);

    // Image Codec
    QStringList supportedImageCaptureCodecs();
    QString imageCaptureCodec();
    void setImageCaptureCodec(const QString &codecName);
    QString imageCaptureCodecDescription(const QString &codecName);

    // Image Quality
    QtMultimediaKit::EncodingQuality captureQuality() const;
    void setCaptureQuality(const QtMultimediaKit::EncodingQuality &quality);

    // S60 3.1 Focus Control (S60 3.2 and later via S60CameraSettings class)
    bool isFocusSupported() const;
    void startFocus();
    void cancelFocus();

    // Zoom Control
    qreal maximumZoom();
    qreal minZoom();
    qreal maxDigitalZoom();
    void doSetZoomFactorL(qreal optical, qreal digital);
    qreal opticalZoomFactor();
    qreal digitalZoomFactor();

    // Exposure Mode Control
    QCameraExposure::ExposureMode exposureMode();
    void setExposureMode(QCameraExposure::ExposureMode mode);
    bool isExposureModeSupported(QCameraExposure::ExposureMode mode) const;

    // Flash Mode Control
    QCameraExposure::FlashMode flashMode();
    void setFlashMode(QCameraExposure::FlashModes mode);
    QCameraExposure::FlashModes supportedFlashModes();

    // Contrast Control
    int contrast() const;
    void setContrast(int value);

    // Brightness Control
    int brightness() const;
    void setBrightness(int value);

    // White Balance Mode Control
    QCameraImageProcessing::WhiteBalanceMode whiteBalanceMode();
    void setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode);
    bool isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const;

public: // Image Decoding & Encoding Notifications

    void handleImageDecoded(int error);
    void handleImageEncoded(int error);

protected: // MCameraEngineObserver

    void MceoFocusComplete();
    void MceoCapturedDataReady(TDesC8* aData);
    void MceoCapturedBitmapReady(CFbsBitmap* aBitmap);
    void MceoHandleError(TCameraEngineError aErrorType, TInt aError);

#ifdef ECAM_PREVIEW_API
protected: // MCameraPreviewObserver

    void MceoPreviewReady(CFbsBitmap& aPreview);
#endif // ECAM_PREVIEW_API

private: // Internal

    QCameraImageCapture::Error fromSymbianErrorToQtMultimediaError(int aError);

    void initializeImageCaptureSettings();
    void resetSession(bool errorHandling = false);

    CCamera::TFormat selectFormatForCodec(const QString &codec);
    CCamera::TFormat defaultImageFormat();
    bool queryCurrentCameraInfo();
    QMap<QString, int> formatMap();
    QMap<QString, QString> codecDescriptionMap();
    void updateImageCaptureFormats();

    void doSetWhiteBalanceModeL(QCameraImageProcessing::WhiteBalanceMode mode);

    void doSetFlashModeL(QCameraExposure::FlashModes mode);
    void doSetExposureModeL(QCameraExposure::ExposureMode mode);

    void saveImageL(TDesC8 *aData, TFileName &aPath);
    void processFileName(const QString &fileName);
    TFileName convertImagePath();

signals: // Notifications

    void stateChanged(QCamera::State);
    void advancedSettingChanged();
    void captureSizeChanged(const QSize&);

    // Error signals
    void cameraError(int, const QString&);          // For QCamera::error
    void captureError(int, int, const QString&);    // For QCameraImageCapture::error

    // Capture notifications
    void readyForCaptureChanged(bool);
    void imageExposed(int);
    void imageCaptured(const int, const QImage&);
    void imageSaved(const int, const QString&);

    // Focus notifications
    void focusStatusChanged(QCamera::LockStatus, QCamera::LockChangeReason);

private slots: // Internal Slots

    void cameraStatusChanged(QCamera::Status);

private: // Data

    CCameraEngine           *m_cameraEngine;
    S60CameraSettings       *m_advancedSettings;
    mutable TCameraInfo     *m_cameraInfo;
    CFbsBitmap              *m_previewBitmap;
    CActiveScheduler        *m_activeScheduler;
    RFs                     *m_fileSystemAccess;
    S60ImageCaptureDecoder  *m_imageDecoder;
    S60ImageCaptureEncoder  *m_imageEncoder;
    mutable int             m_error; // Symbian ErrorCode
    TInt                    m_activeDeviceIndex;
    bool                    m_cameraStarted;
    ImageCaptureState       m_icState;
    QStringList             m_supportedImageCodecs;
    QString                 m_currentCodec;
    CCamera::TFormat        m_currentFormat;
    QSize                   m_captureSize;
    int                     m_symbianImageQuality;
    bool                    m_captureSettingsSet;
    QString                 m_stillCaptureFileName;
    QString                 m_requestedStillCaptureFileName;
    mutable int             m_currentImageId;
    QList<uint>             m_formats;
    // This indicates that image capture should be triggered right after
    // camera and image setting initialization has completed
    bool                    m_captureWhenReady;
    bool                    m_previewDecodingOngoing;
    bool                    m_previewInWaitLoop;
};

#endif // S60IMAGECAPTURESESSION_H

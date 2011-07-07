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

#include <QtCore/qstring.h>
#include <QtCore/qdir.h>

#include "s60imagecapturesession.h"
#include "s60videowidgetcontrol.h"
#include "s60cameraservice.h"
#include "s60cameraconstants.h"

#include <fbs.h>        // CFbsBitmap
#include <pathinfo.h>
#include <imageconversion.h> // ICL Decoder (for SnapShot) & Encoder (for Bitmap Images)

S60ImageCaptureDecoder::S60ImageCaptureDecoder(S60ImageCaptureSession *imageSession,
                                               RFs *fileSystemAccess,
                                               const TDesC8 *data,
                                               const TDesC16 *fileName) :
    CActive(CActive::EPriorityStandard),
    m_imageSession(imageSession),
    m_fs(fileSystemAccess),
    m_jpegImageData(data),
    m_jpegImageFile(fileName),
    m_fileInput(false)
{
    CActiveScheduler::Add(this);
}

S60ImageCaptureDecoder::~S60ImageCaptureDecoder()
{
    if (m_imageDecoder) {
        delete m_imageDecoder;
        m_imageDecoder = 0;
    }
}

S60ImageCaptureDecoder *S60ImageCaptureDecoder::FileNewL(S60ImageCaptureSession *imageSession,
                                                         RFs *fileSystemAccess,
                                                         const TDesC16 *fileName)
{
    S60ImageCaptureDecoder* self = new (ELeave) S60ImageCaptureDecoder(imageSession,
                                                                       fileSystemAccess,
                                                                       0,
                                                                       fileName);
    CleanupStack::PushL(self);
    self->ConstructL(true);
    CleanupStack::Pop(self);
    return self;
}

S60ImageCaptureDecoder *S60ImageCaptureDecoder::DataNewL(S60ImageCaptureSession *imageSession,
                                                         RFs *fileSystemAccess,
                                                         const TDesC8 *data)
{
    S60ImageCaptureDecoder* self = new (ELeave) S60ImageCaptureDecoder(imageSession,
                                                                       fileSystemAccess,
                                                                       data,
                                                                       0);
    CleanupStack::PushL(self);
    self->ConstructL(false);
    CleanupStack::Pop(self);
    return self;
}

void S60ImageCaptureDecoder::ConstructL(const bool fileInput)
{
    if (fileInput) {
        if (!m_imageSession || !m_fs || !m_jpegImageFile)
            User::Leave(KErrGeneral);
        m_imageDecoder = CImageDecoder::FileNewL(*m_fs, *m_jpegImageFile);
    } else {
        if (!m_imageSession || !m_fs || !m_jpegImageData)
            User::Leave(KErrGeneral);
        m_imageDecoder = CImageDecoder::DataNewL(*m_fs, *m_jpegImageData);
    }
}

void S60ImageCaptureDecoder::decode(CFbsBitmap *destBitmap)
{
    if (m_imageDecoder) {
        m_imageDecoder->Convert(&iStatus, *destBitmap, 0);
        SetActive();
    }
    else
        m_imageSession->setError(KErrGeneral, QLatin1String("Preview image creation failed."));
}

TFrameInfo *S60ImageCaptureDecoder::frameInfo()
{
    if (m_imageDecoder) {
        m_frameInfo = m_imageDecoder->FrameInfo();
        return &m_frameInfo;
    }
    else
        return 0;
}

void S60ImageCaptureDecoder::RunL()
{
    m_imageSession->handleImageDecoded(iStatus.Int());
}

void S60ImageCaptureDecoder::DoCancel()
{
    if (m_imageDecoder)
        m_imageDecoder->Cancel();
}

TInt S60ImageCaptureDecoder::RunError(TInt aError)
{
    m_imageSession->setError(aError, QLatin1String("Preview image creation failed."));
    return KErrNone;
}

//=============================================================================

S60ImageCaptureEncoder::S60ImageCaptureEncoder(S60ImageCaptureSession *imageSession,
                                               RFs *fileSystemAccess,
                                               const TDesC16 *fileName,
                                               TInt jpegQuality) :
    CActive(CActive::EPriorityStandard),
    m_imageSession(imageSession),
    m_fileSystemAccess(fileSystemAccess),
    m_fileName(fileName),
    m_jpegQuality(jpegQuality)
{
    CActiveScheduler::Add(this);
}

S60ImageCaptureEncoder::~S60ImageCaptureEncoder()
{
    if (m_frameImageData) {
        delete m_frameImageData;
        m_frameImageData = 0;
    }
    if (m_imageEncoder) {
        delete m_imageEncoder;
        m_imageEncoder = 0;
    }
}

S60ImageCaptureEncoder *S60ImageCaptureEncoder::NewL(S60ImageCaptureSession *imageSession,
                                                     RFs *fileSystemAccess,
                                                     const TDesC16 *fileName,
                                                     TInt jpegQuality)
{
    S60ImageCaptureEncoder* self = new (ELeave) S60ImageCaptureEncoder(imageSession,
                                                                       fileSystemAccess,
                                                                       fileName,
                                                                       jpegQuality);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
}

void S60ImageCaptureEncoder::ConstructL()
{
    if (!m_imageSession || !m_fileSystemAccess || !m_fileName)
        User::Leave(KErrGeneral);

    m_imageEncoder = CImageEncoder::FileNewL(*m_fileSystemAccess,
                                             *m_fileName,
                                             CImageEncoder::EOptionNone,
                                             KImageTypeJPGUid);
    CleanupStack::PushL(m_imageEncoder);

    // Set Jpeg Quality
    m_frameImageData = CFrameImageData::NewL();
    CleanupStack::PushL(m_frameImageData);

    TJpegImageData* jpegFormat = new( ELeave ) TJpegImageData;
    CleanupStack::PushL(jpegFormat);

    jpegFormat->iQualityFactor = m_jpegQuality;

    // jpegFormat (TJpegImageData) ownership transferred to m_frameImageData (CFrameImageData)
    User::LeaveIfError( m_frameImageData->AppendImageData(jpegFormat));

    CleanupStack::Pop(jpegFormat);
    CleanupStack::Pop(m_frameImageData);
    CleanupStack::Pop(m_imageEncoder);
}

void S60ImageCaptureEncoder::encode(CFbsBitmap *sourceBitmap)
{
    if (m_imageEncoder) {
        m_imageEncoder->Convert(&iStatus, *sourceBitmap, m_frameImageData);
        SetActive();
    }
    else
        m_imageSession->setError(KErrGeneral, QLatin1String("Saving image to file failed."));
}

void S60ImageCaptureEncoder::RunL()
{
    m_imageSession->handleImageEncoded(iStatus.Int());
}

void S60ImageCaptureEncoder::DoCancel()
{
    if (m_imageEncoder)
        m_imageEncoder->Cancel();
}

TInt S60ImageCaptureEncoder::RunError(TInt aError)
{
    m_imageSession->setError(aError, QLatin1String("Saving image to file failed."));
    return KErrNone;
}

//=============================================================================

S60ImageCaptureSession::S60ImageCaptureSession(QObject *parent) :
    QObject(parent),
    m_cameraEngine(0),
    m_advancedSettings(0),
    m_cameraInfo(0),
    m_previewBitmap(0),
    m_activeScheduler(0),
    m_fileSystemAccess(0),
    m_imageDecoder(0),
    m_imageEncoder(0),
    m_error(KErrNone),
    m_activeDeviceIndex(KDefaultCameraDevice),
    m_cameraStarted(false),
    m_icState(EImageCaptureNotPrepared),
    m_currentCodec(QString()),
    m_captureSize(QSize()),
    m_symbianImageQuality(QtMultimediaKit::HighQuality * KSymbianImageQualityCoefficient),
    m_captureSettingsSet(false),
    m_stillCaptureFileName(QString()),
    m_requestedStillCaptureFileName(QString()),
    m_currentImageId(0),
    m_captureWhenReady(false),
    m_previewDecodingOngoing(false),
    m_previewInWaitLoop(false)
{
    // Define supported image codecs
    m_supportedImageCodecs << "image/jpeg";

    initializeImageCaptureSettings();

    // Install ActiveScheduler if needed
    if (!CActiveScheduler::Current()) {
        m_activeScheduler = new CActiveScheduler;
        CActiveScheduler::Install(m_activeScheduler);
    }
}

S60ImageCaptureSession::~S60ImageCaptureSession()
{
    // Delete AdvancedSettings (Should already be destroyed by CameraControl)
    deleteAdvancedSettings();

    m_formats.clear();
    m_supportedImageCodecs.clear();

    if (m_imageDecoder) {
        m_imageDecoder->Cancel();
        delete m_imageDecoder;
        m_imageDecoder = 0;
    }
    if (m_imageEncoder) {
        m_imageEncoder->Cancel();
        delete m_imageEncoder;
        m_imageEncoder = 0;
    }

    if (m_previewBitmap) {
        delete m_previewBitmap;
        m_previewBitmap = 0;
    }

    // Uninstall ActiveScheduler if needed
    if (m_activeScheduler) {
        CActiveScheduler::Install(0);
        delete m_activeScheduler;
        m_activeScheduler = 0;
    }
}

CCamera::TFormat S60ImageCaptureSession::defaultImageFormat()
{
    // Primary Camera
    if (m_activeDeviceIndex == 0)
        return KDefaultImageFormatPrimaryCam;

    // Secondary Camera or other
    else
        return KDefaultImageFormatSecondaryCam;
}

bool S60ImageCaptureSession::isDeviceReady()
{
#ifdef Q_CC_NOKIAX86 // Emulator
    return true;
#endif

    if (m_cameraEngine)
        return m_cameraEngine->IsCameraReady();

    return false;
}

void S60ImageCaptureSession::deleteAdvancedSettings()
{
    if (m_advancedSettings) {
        delete m_advancedSettings;
        m_advancedSettings = 0;
        emit advancedSettingChanged();
    }
}

void S60ImageCaptureSession::setCameraHandle(CCameraEngine* camerahandle)
{
    if (camerahandle) {
        m_cameraEngine = camerahandle;
        resetSession();

        // Set default settings
        initializeImageCaptureSettings();
    }
}

void S60ImageCaptureSession::setCurrentDevice(TInt deviceindex)
{
    m_activeDeviceIndex = deviceindex;
}

void S60ImageCaptureSession::notifySettingsSet()
{
    m_captureSettingsSet = true;
}

void S60ImageCaptureSession::resetSession(bool errorHandling)
{
    // Delete old AdvancedSettings
    deleteAdvancedSettings();

    m_captureWhenReady = false;
    m_previewDecodingOngoing = false;
    m_previewInWaitLoop = false;
    m_stillCaptureFileName = QString();
    m_requestedStillCaptureFileName = QString();
    m_icState = EImageCaptureNotPrepared;

    m_error = KErrNone;
    m_currentFormat = defaultImageFormat();

    int err = KErrNone;
    m_advancedSettings = S60CameraSettings::New(err, this, m_cameraEngine);
    if (err == KErrNotSupported) {
        m_advancedSettings = 0;
#ifndef S60_31_PLATFORM // Post S60 3.1 Platform
        // Adv. settings may not be supported for other than the Primary Camera
        if (m_cameraEngine->CurrentCameraIndex() == 0)
            setError(err, tr("Unexpected camera error."));
#endif // !S60_31_PLATFORM
    } else if (err != KErrNone) { // Other errors
        m_advancedSettings = 0;
        qWarning("Failed to create camera settings handler.");
        if (errorHandling)
            emit cameraError(QCamera::ServiceMissingError, tr("Failed to recover from error."));
        else
            setError(err, tr("Unexpected camera error."));
        return;
    }

    if (m_advancedSettings) {
        if (m_cameraEngine)
            m_cameraEngine->SetAdvancedObserver(m_advancedSettings);
        else
            setError(KErrNotReady, tr("Unexpected camera error."));
    }

    updateImageCaptureFormats();

    emit advancedSettingChanged();
}

S60CameraSettings* S60ImageCaptureSession::advancedSettings()
{
    return m_advancedSettings;
}

/*
 * This function can be used both internally and from Control classes using
 * this session. The error notification will go to the client application
 * either through QCameraImageCapture (if captureError is true) or QCamera (if
 * captureError is false, default) error signal.
 */
void S60ImageCaptureSession::setError(const TInt error,
                                      const QString &description,
                                      const bool captureError)
{
    if (error == KErrNone)
        return;

    m_error = error;
    QCameraImageCapture::Error cameraError = fromSymbianErrorToQtMultimediaError(error);

    if (captureError) {
        emit this->captureError(m_currentImageId, cameraError, description);
        if (cameraError != QCameraImageCapture::NotSupportedFeatureError)
            resetSession(true);
    } else {
        emit this->cameraError(cameraError, description);
        if (cameraError != QCamera::NotSupportedFeatureError)
            resetSession(true);
    }
}

QCameraImageCapture::Error S60ImageCaptureSession::fromSymbianErrorToQtMultimediaError(int aError)
{
    switch(aError) {
        case KErrNone:
            return QCameraImageCapture::NoError; // No errors have occurred
        case KErrNotReady:
            return QCameraImageCapture::NotReadyError; // Not ready for operation
        case KErrNotSupported:
            return QCameraImageCapture::NotSupportedFeatureError; // The feature is not supported
        case KErrNoMemory:
            return QCameraImageCapture::OutOfSpaceError; // Out of disk space
        case KErrNotFound:
        case KErrBadHandle:
            return QCameraImageCapture::ResourceError; // No resources available

        default:
            return QCameraImageCapture::ResourceError; // Other error has occurred
    }
}

int S60ImageCaptureSession::currentImageId() const
{
    return m_currentImageId;
}

void S60ImageCaptureSession::initializeImageCaptureSettings()
{
    if (m_captureSettingsSet)
        return;

    m_currentCodec = KDefaultImageCodec;
    m_captureSize = QSize(-1, -1);
    m_currentFormat = defaultImageFormat();

    // Resolution
    if (m_cameraEngine) {
        QList<QSize> resolutions = supportedCaptureSizesForCodec(imageCaptureCodec());
        foreach (QSize reso, resolutions) {
            if ((reso.width() * reso.height()) > (m_captureSize.width() * m_captureSize.height()))
                m_captureSize = reso;
        }
    } else {
        m_captureSize = KDefaultImageResolution;
    }

    m_symbianImageQuality = KDefaultImageQuality;
}

/*
 * This function selects proper format to be used for the captured image based
 * on the requested image codec.
 */
CCamera::TFormat S60ImageCaptureSession::selectFormatForCodec(const QString &codec)
{
    CCamera::TFormat format = CCamera::EFormatMonochrome;

    if (codec == "image/jpg" || codec == "image/jpeg") {
        // Primary Camera
        if (m_activeDeviceIndex == 0)
            format = KDefaultImageFormatPrimaryCam;

        // Secondary Camera or other
        else
            format = KDefaultImageFormatSecondaryCam;

        return format;
    }

    setError(KErrNotSupported, tr("Failed to select color format to be used with image codec."));
    return format;
}

int S60ImageCaptureSession::prepareImageCapture()
{
    if (m_cameraEngine) {
        if (!m_cameraEngine->IsCameraReady()) {
            // Reset state to make sure camera is prepared before capturing image
            m_icState = EImageCaptureNotPrepared;
            return KErrNotReady;
        }

        // First set the quality
        CCamera *camera = m_cameraEngine->Camera();
        if (camera)
            camera->SetJpegQuality(m_symbianImageQuality);
        else
            setError(KErrNotReady, tr("Setting image quality failed."), true);

        // Then prepare with correct resolution and format
        TSize captureSize = TSize(m_captureSize.width(), m_captureSize.height());
        TRAPD(symbianError, m_cameraEngine->PrepareL(captureSize, m_currentFormat));
        if (!symbianError)
            m_icState = EImageCapturePrepared;

        // Check if CaptureSize was modified
        if (captureSize.iWidth != m_captureSize.width() || captureSize.iHeight != m_captureSize.height())
            m_captureSize = QSize(captureSize.iWidth, captureSize.iHeight);
        emit captureSizeChanged(m_captureSize);

#ifdef ECAM_PREVIEW_API
        // Subscribe previews
        MCameraPreviewObserver *observer = this;
        m_cameraEngine->EnablePreviewProvider(observer);
#endif // ECAM_PREVIEW_API

        return symbianError;
    }

    return KErrGeneral;
}

void S60ImageCaptureSession::releaseImageCapture()
{
    // Make sure ImageCapture is prepared the next time it is being activated
    m_icState = EImageCaptureNotPrepared;

#ifdef ECAM_PREVIEW_API
    // Cancel preview subscription
    m_cameraEngine->DisablePreviewProvider();
#endif // ECAM_PREVIEW_API
}

int S60ImageCaptureSession::capture(const QString &fileName)
{
    if (!m_cameraStarted) {
        m_captureWhenReady = true;
        m_requestedStillCaptureFileName = fileName; // Save name, it will be processed during actual capture
        return 0;
    }

    if (m_icState < EImageCapturePrepared) {
        int prepareSuccess = prepareImageCapture();
        if (prepareSuccess) {
            setError(prepareSuccess, tr("Failure during image capture preparation."), true);
            return 0;
        }
    } else if (m_icState > EImageCapturePrepared) {
        setError(KErrNotReady, tr("Previous operation is still ongoing."), true);
        return 0;
    }

    m_icState = EImageCaptureCapturing;

    // Give new ID for the new image
    m_currentImageId += 1;

    emit readyForCaptureChanged(false);

    processFileName(fileName);

    if (m_cameraEngine) {
        TRAPD(err, m_cameraEngine->CaptureL());
        setError(err, tr("Image capture failed."), true);
    } else {
        setError(KErrNotReady, tr("Unexpected camera error."), true);
    }

#ifdef Q_CC_NOKIAX86 // Emulator
    QImage *snapImage = new QImage(QLatin1String("C:/Data/testimage.jpg"));
    emit imageExposed(m_currentImageId);
    emit imageCaptured(m_currentImageId, *snapImage);
    emit imageSaved(m_currentImageId, m_stillCaptureFileName);
#endif // Q_CC_NOKIAX86

    return m_currentImageId;
}

void S60ImageCaptureSession::cancelCapture()
{
    if (m_icState != EImageCaptureCapturing)
        return;

    if (m_cameraEngine)
        m_cameraEngine->CancelCapture();

    m_icState = EImageCapturePrepared;
}

void S60ImageCaptureSession::processFileName(const QString &fileName)
{
    // Empty FileName - Use default file name and path (C:\Data\Images\image.jpg)
    if (fileName.isEmpty()) {
        // Make sure default directory exists
        QDir videoDir(QDir::rootPath());
        if (!videoDir.exists(KDefaultImagePath))
            videoDir.mkpath(KDefaultImagePath);
        QString defaultFile = KDefaultImagePath;
        defaultFile.append("\\");
        defaultFile.append(KDefaultImageFileName);
        m_stillCaptureFileName = defaultFile;

    } else { // Not empty

        QString fullFileName;

        // Relative FileName
        if (!fileName.contains(":")) {
            // Extract file name and path from the URL
            fullFileName = KDefaultImagePath;
            if (fileName.at(0) != '\\')
                fullFileName.append("\\");
            fullFileName.append(QDir::toNativeSeparators(QDir::cleanPath(fileName)));

        // Absolute FileName
        } else {
            // Extract file name and path from the given location
            fullFileName = QDir::toNativeSeparators(QDir::cleanPath(fileName));
        }

        QString fileNameOnly = fullFileName.right(fullFileName.length() - fullFileName.lastIndexOf("\\") - 1);
        QString directory = fullFileName.left(fullFileName.lastIndexOf("\\"));
        if (directory.lastIndexOf("\\") == (directory.length() - 1))
            directory = directory.left(directory.length() - 1);

        // URL is Absolute path, not including file name
        if (!fileNameOnly.contains(".")) {
            if (fileNameOnly != "") {
                directory.append("\\");
                directory.append(fileNameOnly);
            }
            fileNameOnly = KDefaultImageFileName;
        }

        // Make sure absolute directory exists
        QDir imageDir(QDir::rootPath());
        if (!imageDir.exists(directory))
            imageDir.mkpath(directory);

        QString resolvedFileName = directory;
        resolvedFileName.append("\\");
        resolvedFileName.append(fileNameOnly);
        m_stillCaptureFileName = resolvedFileName;
    }
}

void S60ImageCaptureSession::MceoFocusComplete()
{
    emit focusStatusChanged(QCamera::Locked, QCamera::LockAcquired);
}

void S60ImageCaptureSession::MceoCapturedDataReady(TDesC8* aData)
{
    emit imageExposed(m_currentImageId);

    m_icState = EImageCaptureWritingImage;

    TFileName path = convertImagePath();

    // Try to save image and inform if it was succcesful
    TRAPD(err, saveImageL(aData, path));
    if (err) {
        if (m_previewDecodingOngoing)
            m_previewDecodingOngoing = false; // Reset

        setError(err, tr("Writing captured image to a file failed."), true);
        m_icState = EImageCapturePrepared;
        return;
    }

    m_icState = EImageCapturePrepared;

}

void S60ImageCaptureSession::MceoCapturedBitmapReady(CFbsBitmap* aBitmap)
{
    emit imageExposed(m_currentImageId);

    m_icState = EImageCaptureWritingImage;

    if(aBitmap)
    {
#ifndef ECAM_PREVIEW_API
        if (m_previewDecodingOngoing) {
            m_previewInWaitLoop = true;
            CActiveScheduler::Start(); // Wait for the completion of the previous Preview generation
        }

        // Delete old instances if needed
        if (m_imageDecoder) {
            delete m_imageDecoder;
            m_imageDecoder = 0;
        }
        if (m_previewBitmap) {
            delete m_previewBitmap;
            m_previewBitmap = 0;
        }
#endif // ECAM_CAMERA_API
        if (m_imageEncoder) {
            delete m_imageEncoder;
            m_imageEncoder = 0;
        }
        if (m_fileSystemAccess) {
            m_fileSystemAccess->Close();
            delete m_fileSystemAccess;
            m_fileSystemAccess = 0;
        }

        TInt saveError = KErrNone;
        TFileName path = convertImagePath();

        // Create FileSystem access
        m_fileSystemAccess = new RFs;
        if (!m_fileSystemAccess) {
            setError(KErrNoMemory, tr("Failed to write captured image to a file."));
            return;
        }
        saveError = m_fileSystemAccess->Connect();
        if (saveError) {
            setError(saveError, tr("Failed to write captured image to a file."));
            return;
        }

        TRAP(saveError, m_imageEncoder = S60ImageCaptureEncoder::NewL(this,
                                                                      m_fileSystemAccess,
                                                                      &path,
                                                                      m_symbianImageQuality));
        if (saveError)
            setError(saveError, tr("Saving captured image failed."), true);
        m_previewDecodingOngoing = true;
        m_imageEncoder->encode(aBitmap);

    } else {
        setError(KErrBadHandle, tr("Unexpected camera error."), true);
    }

    m_icState = EImageCapturePrepared;
}

void S60ImageCaptureSession::MceoHandleError(TCameraEngineError aErrorType, TInt aError)
{
    Q_UNUSED(aErrorType);
    setError(aError, tr("General camera error."));
}

TFileName S60ImageCaptureSession::convertImagePath()
{
    TFileName path = KNullDesC();

    // Convert to Symbian path
    TPtrC16 attachmentPath(KNullDesC);

    // Path is already included in filename
    attachmentPath.Set(reinterpret_cast<const TUint16*>(QDir::toNativeSeparators(m_stillCaptureFileName).utf16()));
    path.Append(attachmentPath);

    return path;
}

/*
 * Creates (asynchronously) Preview Image from Jpeg ImageBuffer and also
 * writes Jpeg (synchronously) to a file.
 */
void S60ImageCaptureSession::saveImageL(TDesC8 *aData, TFileName &aPath)
{
    if (aData == 0)
        setError(KErrGeneral, tr("Captured image data is not available."), true);

    if (aPath.Size() > 0) {
#ifndef ECAM_PREVIEW_API
        if (m_previewDecodingOngoing) {
            m_previewInWaitLoop = true;
            CActiveScheduler::Start(); // Wait for the completion of the previous Preview generation
        }

        // Delete old instances if needed
        if (m_imageDecoder) {
            delete m_imageDecoder;
            m_imageDecoder = 0;
        }
        if (m_previewBitmap) {
            delete m_previewBitmap;
            m_previewBitmap = 0;
        }
#endif // ECAM_PREVIEW_API
        if (m_fileSystemAccess) {
            m_fileSystemAccess->Close();
            delete m_fileSystemAccess;
            m_fileSystemAccess = 0;
        }

        RFs *fileSystemAccess = new (ELeave) RFs;
        User::LeaveIfError(fileSystemAccess->Connect());
        CleanupClosePushL(*fileSystemAccess);

#ifndef ECAM_PREVIEW_API
        // Generate Thumbnail to be used as Preview
        S60ImageCaptureDecoder *imageDecoder = S60ImageCaptureDecoder::DataNewL(this, fileSystemAccess, aData);
        CleanupStack::PushL(imageDecoder);

        // Set proper Preview Size
        TSize scaledSize((m_captureSize.width() / KSnapshotDownScaleFactor), (m_captureSize.height() / KSnapshotDownScaleFactor));
        if (scaledSize.iWidth < KSnapshotMinWidth || scaledSize.iHeight < KSnapshotMinHeight)
            scaledSize.SetSize((m_captureSize.width() / (KSnapshotDownScaleFactor/2)), (m_captureSize.height() / (KSnapshotDownScaleFactor/2)));
        if (scaledSize.iWidth < KSnapshotMinWidth || scaledSize.iHeight < KSnapshotMinHeight)
            scaledSize.SetSize((m_captureSize.width() / (KSnapshotDownScaleFactor/4)), (m_captureSize.height() / (KSnapshotDownScaleFactor/4)));
        if (scaledSize.iWidth < KSnapshotMinWidth || scaledSize.iHeight < KSnapshotMinHeight)
            scaledSize.SetSize(m_captureSize.width(), m_captureSize.height());

        TFrameInfo *info = imageDecoder->frameInfo();
        if (!info) {
            setError(KErrGeneral, tr("Preview image creation failed."));
            return;
        }

        CFbsBitmap *previewBitmap = new (ELeave) CFbsBitmap;
        CleanupStack::PushL(previewBitmap);
        TInt bitmapCreationErr = previewBitmap->Create(scaledSize, info->iFrameDisplayMode);
        if (bitmapCreationErr) {
            setError(bitmapCreationErr, tr("Preview creation failed."));
            return;
        }

        // Jpeg conversion completes in RunL
        m_previewDecodingOngoing = true;
        imageDecoder->decode(previewBitmap);
#endif // ECAM_PREVIEW_API

        RFile file;
        TInt fileWriteErr = KErrNone;
        fileWriteErr = file.Replace(*fileSystemAccess, aPath, EFileWrite);
        if (fileWriteErr)
            User::Leave(fileWriteErr);
        CleanupClosePushL(file); // Close if Leaves

        fileWriteErr = file.Write(*aData);
        if (fileWriteErr)
            User::Leave(fileWriteErr);

        CleanupStack::PopAndDestroy(&file);
#ifdef ECAM_PREVIEW_API
        CleanupStack::PopAndDestroy(fileSystemAccess);
#else // !ECAM_PREVIEW_API
        // Delete when Image is decoded
        CleanupStack::Pop(previewBitmap);
        CleanupStack::Pop(imageDecoder);
        CleanupStack::Pop(fileSystemAccess);

        // Set member variables (Cannot leave any more)
        m_previewBitmap = previewBitmap;
        m_imageDecoder = imageDecoder;
        m_fileSystemAccess = fileSystemAccess;
#endif // ECAM_PREVIEW_API

        emit imageSaved(m_currentImageId, m_stillCaptureFileName);

        // Inform that we can continue taking more pictures
        emit readyForCaptureChanged(true);

        // For custom preview generation, image buffer gets released in RunL()
#ifdef ECAM_PREVIEW_API
        releaseImageBuffer();
#endif // ECAM_PREVIEW_API

    } else {
        setError(KErrPathNotFound, tr("Invalid path given."), true);
    }
}

void S60ImageCaptureSession::releaseImageBuffer()
{
    if (m_cameraEngine)
        m_cameraEngine->ReleaseImageBuffer();
    else
        setError(KErrNotReady, tr("Unexpected camera error."), true);
}

/*
 * Queries camera properties
 * Results are returned to member variable m_info
 *
 * @return boolean indicating if querying the info was a success
 */
bool S60ImageCaptureSession::queryCurrentCameraInfo()
{
    if (m_cameraEngine) {
        m_cameraInfo = m_cameraEngine->CameraInfo();
        return true;
    }

    return false;
}

/*
 * This function handles different camera status changes
 */
void S60ImageCaptureSession::cameraStatusChanged(QCamera::Status status)
{
    if (status == QCamera::ActiveStatus) {
        m_cameraStarted = true;
        if (m_captureWhenReady)
            capture(m_requestedStillCaptureFileName);
    }else if (status == QCamera::UnloadedStatus) {
        m_cameraStarted = false;
        m_icState = EImageCaptureNotPrepared;
    }
    else
        m_cameraStarted = false;
}

QSize S60ImageCaptureSession::captureSize() const
{
    return m_captureSize;
}

QSize S60ImageCaptureSession::minimumCaptureSize()
{
    return supportedCaptureSizesForCodec(formatMap().key(m_currentFormat)).first();
}
QSize S60ImageCaptureSession::maximumCaptureSize()
{
    return supportedCaptureSizesForCodec(formatMap().key(m_currentFormat)).last();
}

void S60ImageCaptureSession::setCaptureSize(const QSize &size)
{
    if (size.isNull() ||
        size.isEmpty() ||
        size == QSize(-1,-1)) {
        // An empty QSize indicates the encoder should make an optimal choice based on what is
        // available from the image source and the limitations of the codec.
        m_captureSize = supportedCaptureSizesForCodec(formatMap().key(m_currentFormat)).last();
    }
    else
        m_captureSize = size;
}

QList<QSize> S60ImageCaptureSession::supportedCaptureSizesForCodec(const QString &codecName)
{
    QList<QSize> list;

    // If we have CameraEngine loaded and we can update CameraInfo
    if (m_cameraEngine && queryCurrentCameraInfo()) {
        CCamera::TFormat format;
        if (codecName == "")
            format = defaultImageFormat();
        else
            format = selectFormatForCodec(codecName);

        CCamera *camera = m_cameraEngine->Camera();
        TSize imageSize;
        if (camera) {
            for (int i = 0; i < m_cameraInfo->iNumImageSizesSupported; i++) {
                camera->EnumerateCaptureSizes(imageSize, i, format);
                list << QSize(imageSize.iWidth, imageSize.iHeight); // Add resolution to the list
            }
        }
    }

#ifdef Q_CC_NOKIAX86 // Emulator
    // Add some for testing purposes
    list << QSize(50, 50);
    list << QSize(100, 100);
    list << QSize(800,600);
#endif

    return list;
}

QMap<QString, int> S60ImageCaptureSession::formatMap()
{
    QMap<QString, int> formats;

    // Format list copied from CCamera::TFormat (in ecam.h)
    formats.insert("Monochrome",        0x0001);
    formats.insert("16bitRGB444",       0x0002);
    formats.insert("16BitRGB565",       0x0004);
    formats.insert("32BitRGB888",       0x0008);
    formats.insert("Jpeg",              0x0010);
    formats.insert("Exif",              0x0020);
    formats.insert("FbsBitmapColor4K",  0x0040);
    formats.insert("FbsBitmapColor64K", 0x0080);
    formats.insert("FbsBitmapColor16M", 0x0100);
    formats.insert("UserDefined",       0x0200);
    formats.insert("YUV420Interleaved", 0x0400);
    formats.insert("YUV420Planar",      0x0800);
    formats.insert("YUV422",            0x1000);
    formats.insert("YUV422Reversed",    0x2000);
    formats.insert("YUV444",            0x4000);
    formats.insert("YUV420SemiPlanar",  0x8000);
    formats.insert("FbsBitmapColor16MU", 0x00010000);
    formats.insert("MJPEG",             0x00020000);
    formats.insert("EncodedH264",       0x00040000);

    return formats;
}

QMap<QString, QString> S60ImageCaptureSession::codecDescriptionMap()
{
    QMap<QString, QString> formats;

    formats.insert("image/jpg", "JPEG image codec");

    return formats;
}

QStringList S60ImageCaptureSession::supportedImageCaptureCodecs()
{
#ifdef Q_CC_NOKIAX86 // Emulator
    return formatMap().keys();
#endif

    return m_supportedImageCodecs;
}

void S60ImageCaptureSession::updateImageCaptureFormats()
{
    m_formats.clear();
    if (m_cameraEngine && queryCurrentCameraInfo()) {
        TUint32 supportedFormats = m_cameraInfo->iImageFormatsSupported;

#ifdef S60_3X_PLATFORM // S60 3.1 & 3.2
        int maskEnd = CCamera::EFormatFbsBitmapColor16MU;
#else // S60 5.0 or later
        int maskEnd = CCamera::EFormatEncodedH264;
#endif // S60_3X_PLATFORM

        for (int mask = CCamera::EFormatMonochrome; mask <= maskEnd; mask <<= 1) {
            if (supportedFormats & mask)
                m_formats << mask; // Store mask of supported format
        }
    }
}

QString S60ImageCaptureSession::imageCaptureCodec()
{
    return m_currentCodec;
}
void S60ImageCaptureSession::setImageCaptureCodec(const QString &codecName)
{
    if (!codecName.isEmpty()) {
        if (supportedImageCaptureCodecs().contains(codecName, Qt::CaseInsensitive) ||
            codecName == "image/jpg") {
            m_currentCodec = codecName;
            m_currentFormat = selectFormatForCodec(m_currentCodec);
        } else {
            setError(KErrNotSupported, tr("Requested image codec is not supported"));
        }
    } else {
        m_currentCodec = KDefaultImageCodec;
        m_currentFormat = selectFormatForCodec(m_currentCodec);
    }
}

QString S60ImageCaptureSession::imageCaptureCodecDescription(const QString &codecName)
{
    QString description = codecDescriptionMap().value(codecName);
    return description;
}

QtMultimediaKit::EncodingQuality S60ImageCaptureSession::captureQuality() const
{
    switch (m_symbianImageQuality) {
        case KJpegQualityVeryLow:
            return QtMultimediaKit::VeryLowQuality;
        case KJpegQualityLow:
            return QtMultimediaKit::LowQuality;
        case KJpegQualityNormal:
            return QtMultimediaKit::NormalQuality;
        case KJpegQualityHigh:
            return QtMultimediaKit::HighQuality;
        case KJpegQualityVeryHigh:
            return QtMultimediaKit::VeryHighQuality;

        default:
            // Return normal as default
            return QtMultimediaKit::NormalQuality;
    }
}

void S60ImageCaptureSession::setCaptureQuality(const QtMultimediaKit::EncodingQuality &quality)
{
    // Use sensible presets
    switch (quality) {
        case QtMultimediaKit::VeryLowQuality:
            m_symbianImageQuality = KJpegQualityVeryLow;
            break;
        case QtMultimediaKit::LowQuality:
            m_symbianImageQuality = KJpegQualityLow;
            break;
        case QtMultimediaKit::NormalQuality:
            m_symbianImageQuality = KJpegQualityNormal;
            break;
        case QtMultimediaKit::HighQuality:
            m_symbianImageQuality = KJpegQualityHigh;
            break;
        case QtMultimediaKit::VeryHighQuality:
            m_symbianImageQuality = KJpegQualityVeryHigh;
            break;

        default:
            m_symbianImageQuality = quality * KSymbianImageQualityCoefficient;
            break;
    }
}

qreal S60ImageCaptureSession::maximumZoom()
{
    qreal maxZoomFactor = 1.0;

    if (queryCurrentCameraInfo()) {
        maxZoomFactor = m_cameraInfo->iMaxZoomFactor;

        if (maxZoomFactor == 0.0 || maxZoomFactor == 1.0) {
            return 1.0; // Not supported
        } else {
            return maxZoomFactor;
        }
    } else {
        return 1.0;
    }
}

qreal S60ImageCaptureSession::minZoom()
{
    qreal minZoomValue = 1.0;

    if (queryCurrentCameraInfo()) {
        minZoomValue = m_cameraInfo->iMinZoomFactor;
        if (minZoomValue == 0.0 || minZoomValue == 1.0)
            return 1.0; // Macro Zoom is not supported
        else {
            return minZoomValue;
        }

    } else {
        return 1.0;
    }
}

qreal S60ImageCaptureSession::maxDigitalZoom()
{
    qreal maxDigitalZoomFactor = 1.0;

    if (queryCurrentCameraInfo()) {
        maxDigitalZoomFactor = m_cameraInfo->iMaxDigitalZoomFactor;
        return maxDigitalZoomFactor;
    } else {
        return 1.0;
    }
}

void S60ImageCaptureSession::doSetZoomFactorL(qreal optical, qreal digital)
{
#if !defined(USE_S60_32_ECAM_ADVANCED_SETTINGS_HEADER) & !defined(USE_S60_50_ECAM_ADVANCED_SETTINGS_HEADER)
    // Convert Zoom Factor to Zoom Value if AdvSettings are not available
    int digitalSymbian = (digital * m_cameraInfo->iMaxDigitalZoom) / maxDigitalZoom();
    if (m_cameraInfo->iMaxDigitalZoom != 0 && digital == 1.0)
        digitalSymbian = 1; // Make sure zooming out to initial value if requested
#endif // !USE_S60_32_ECAM_ADVANCED_SETTINGS_HEADER & !USE_S60_50_ECAM_ADVANCED_SETTINGS_HEADER

    if (m_cameraEngine && !m_cameraEngine->IsCameraReady())
        return;

    if (m_cameraEngine && queryCurrentCameraInfo()) {
        CCamera *camera = m_cameraEngine->Camera();
        if (camera) {

            // Optical Zoom
            if (!qFuzzyCompare(optical, qreal(1.0)) && !qFuzzyCompare(optical, qreal(0.0))) {
                setError(KErrNotSupported, tr("Requested optical zoom factor is not supported."));
                return;
            }

            // Digital Zoom (Smooth Zoom - Zoom value set in steps)
            if (digital != digitalZoomFactor()) {
                if ((digital > 1.0 || qFuzzyCompare(digital, qreal(1.0))) &&
                    digital <= m_cameraInfo->iMaxDigitalZoomFactor) {
#if defined(USE_S60_32_ECAM_ADVANCED_SETTINGS_HEADER) | defined(USE_S60_50_ECAM_ADVANCED_SETTINGS_HEADER)
                    if (m_advancedSettings) {
                        qreal currentZoomFactor = m_advancedSettings->digitalZoomFactorL();

                        QList<qreal> smoothZoomSetValues;
                        QList<qreal> factors = m_advancedSettings->supportedDigitalZoomFactors();
                        if (currentZoomFactor < digital) {
                            for (int i = 0; i < factors.count(); ++i) {
                                if (factors.at(i) > currentZoomFactor && factors.at(i) < digital)
                                    smoothZoomSetValues << factors.at(i);
                            }

                            for (int i = 0; i < smoothZoomSetValues.count(); i = i + KSmoothZoomStep) {
                                m_advancedSettings->setDigitalZoomFactorL(smoothZoomSetValues[i]); // Using Zoom Factor
                            }

                        } else {
                            for (int i = 0; i < factors.count(); ++i) {
                                if (factors.at(i) < currentZoomFactor && factors.at(i) > digital)
                                    smoothZoomSetValues << factors.at(i);
                            }

                            for (int i = (smoothZoomSetValues.count() - 1); i >= 0; i = i - KSmoothZoomStep) {
                                m_advancedSettings->setDigitalZoomFactorL(smoothZoomSetValues[i]); // Using Zoom Factor
                            }
                        }

                        // Set final value
                        m_advancedSettings->setDigitalZoomFactorL(digital);
                    }
                    else
                        setError(KErrNotReady, tr("Zooming failed."));
#else // No advanced settigns
                    // Define zoom steps
                    int currentZoomFactor = camera->DigitalZoomFactor();
                    int difference = abs(currentZoomFactor - digitalSymbian);
                    int midZoomValue = currentZoomFactor;

                    if (currentZoomFactor < digitalSymbian) {
                        while (midZoomValue < (digitalSymbian - KSmoothZoomStep)) {
                            midZoomValue = midZoomValue + KSmoothZoomStep;
                            camera->SetDigitalZoomFactorL(midZoomValue);
                        }
                    } else {
                        while (midZoomValue > (digitalSymbian + KSmoothZoomStep)) {
                            midZoomValue = midZoomValue - KSmoothZoomStep;
                            camera->SetDigitalZoomFactorL(midZoomValue);
                        }
                    }

                    // Set final and emit signal
                    camera->SetDigitalZoomFactorL(digitalSymbian);
#endif // USE_S60_32_ECAM_ADVANCED_SETTINGS_HEADER | USE_S60_50_ECAM_ADVANCED_SETTINGS_HEADER
                } else {
                    setError(KErrNotSupported, tr("Requested digital zoom factor is not supported."));
                    return;
                }
            }
        }
    } else {
        setError(KErrGeneral, tr("Unexpected camera error."));
    }
}

qreal S60ImageCaptureSession::opticalZoomFactor()
{
    qreal factor = 1.0;

#if defined(USE_S60_32_ECAM_ADVANCED_SETTINGS_HEADER) | defined(USE_S60_50_ECAM_ADVANCED_SETTINGS_HEADER)
    if (m_advancedSettings) {
        TRAPD(err, factor = m_advancedSettings->opticalZoomFactorL());
        if (err)
            return 1.0;
    }
#else // No advanced settigns
    if (m_cameraEngine && m_cameraInfo) {
        if (m_cameraEngine->Camera()) {
            if (m_cameraInfo->iMaxZoom != 0)
                factor = (m_cameraEngine->Camera()->ZoomFactor()* maximumZoom()) / m_cameraInfo->iMaxZoom;
            else
                factor = 1.0;
        }
    }
#endif // USE_S60_32_ECAM_ADVANCED_SETTINGS_HEADER | USE_S60_50_ECAM_ADVANCED_SETTINGS_HEADER

    if (factor == 0.0) // If not supported
        factor = 1.0;

    return factor;
}

qreal S60ImageCaptureSession::digitalZoomFactor()
{
    qreal factor = 1.0;

#if defined(USE_S60_32_ECAM_ADVANCED_SETTINGS_HEADER) | defined(USE_S60_50_ECAM_ADVANCED_SETTINGS_HEADER)
    if (m_advancedSettings) {
        TRAPD(err, factor = m_advancedSettings->digitalZoomFactorL());
        if (err)
            return 1.0;
    }
#else // No advanced settigns
    if (m_cameraEngine && m_cameraInfo) {
        if (m_cameraEngine->Camera()) {
            if (m_cameraInfo->iMaxDigitalZoom != 0)
                factor = (m_cameraEngine->Camera()->DigitalZoomFactor()* maxDigitalZoom()) / m_cameraInfo->iMaxDigitalZoom;
            else
                factor = 1.0;
        }
    }
#endif // USE_S60_32_ECAM_ADVANCED_SETTINGS_HEADER | USE_S60_50_ECAM_ADVANCED_SETTINGS_HEADER

    if (factor == 0.0)
        factor = 1.0;

    return factor;
}

void S60ImageCaptureSession::setFlashMode(QCameraExposure::FlashModes mode)
{
    TRAPD(err, doSetFlashModeL(mode));
    setError(err, tr("Failed to set flash mode."));
}

void S60ImageCaptureSession::doSetFlashModeL(QCameraExposure::FlashModes mode)
{
    if (m_cameraEngine && m_cameraEngine->Camera()) {
        CCamera *camera = m_cameraEngine->Camera();
        switch(mode) {
            case QCameraExposure::FlashOff:
                camera->SetFlashL(CCamera::EFlashNone);
                break;
            case QCameraExposure::FlashAuto:
                camera->SetFlashL(CCamera::EFlashAuto);
                break;
            case QCameraExposure::FlashOn:
                camera->SetFlashL(CCamera::EFlashForced);
                break;
            case QCameraExposure::FlashRedEyeReduction:
                camera->SetFlashL(CCamera::EFlashRedEyeReduce);
                break;
            case QCameraExposure::FlashFill:
                camera->SetFlashL(CCamera::EFlashFillIn);
                break;

            default:
                setError(KErrNotSupported, tr("Requested flash mode is not suported"));
                break;
        }
    } else {
        setError(KErrNotReady, tr("Unexpected camera error."));
    }
}

QCameraExposure::FlashMode S60ImageCaptureSession::flashMode()
{
    if (m_cameraEngine && m_cameraEngine->Camera()) {
        CCamera *camera = m_cameraEngine->Camera();
        switch(camera->Flash()) {
            case CCamera::EFlashAuto:
                return QCameraExposure::FlashAuto;
            case CCamera::EFlashForced:
                return QCameraExposure::FlashOn;
            case CCamera::EFlashRedEyeReduce:
                return QCameraExposure::FlashRedEyeReduction;
            case CCamera::EFlashFillIn:
                return QCameraExposure::FlashFill;
            case CCamera::EFlashNone:
                return QCameraExposure::FlashOff;

            default:
                return QCameraExposure::FlashAuto; // Most probable default
        }
    } else {
        setError(KErrNotReady, tr("Unexpected camera error."));
    }

    return QCameraExposure::FlashOff;
}

QCameraExposure::FlashModes S60ImageCaptureSession::supportedFlashModes()
{
    QCameraExposure::FlashModes modes = QCameraExposure::FlashOff;

    if (queryCurrentCameraInfo()) {
        TInt supportedModes = m_cameraInfo->iFlashModesSupported;

        if (supportedModes == 0)
            return modes;

        if (supportedModes & CCamera::EFlashManual)
             modes |= QCameraExposure::FlashOff;
        if (supportedModes & CCamera::EFlashForced)
             modes |= QCameraExposure::FlashOn;
        if (supportedModes & CCamera::EFlashAuto)
             modes |= QCameraExposure::FlashAuto;
        if (supportedModes & CCamera::EFlashFillIn)
             modes |= QCameraExposure::FlashFill;
        if (supportedModes & CCamera::EFlashRedEyeReduce)
             modes |= QCameraExposure::FlashRedEyeReduction;
    }

    return modes;
}

QCameraExposure::ExposureMode S60ImageCaptureSession::exposureMode()
{
    if (m_cameraEngine && m_cameraEngine->Camera()) {
        CCamera* camera = m_cameraEngine->Camera();
        switch(camera->Exposure()) {
            case CCamera::EExposureManual:
                return QCameraExposure::ExposureManual;
            case CCamera::EExposureAuto:
                return QCameraExposure::ExposureAuto;
            case CCamera::EExposureNight:
                return QCameraExposure::ExposureNight;
            case CCamera::EExposureBacklight:
                return QCameraExposure::ExposureBacklight;
            case CCamera::EExposureSport:
                return QCameraExposure::ExposureSports;
            case CCamera::EExposureSnow:
                return QCameraExposure::ExposureSnow;
            case CCamera::EExposureBeach:
                return QCameraExposure::ExposureBeach;

            default:
                return QCameraExposure::ExposureAuto;
        }
    }

    return QCameraExposure::ExposureAuto;
}

bool S60ImageCaptureSession::isExposureModeSupported(QCameraExposure::ExposureMode mode) const
{
    TInt supportedModes = m_cameraInfo->iExposureModesSupported;

    if (supportedModes == 0)
        return false;

    switch (mode) {
        case QCameraExposure::ExposureManual:
            if(supportedModes & CCamera::EExposureManual)
                return true;
            else
                return false;
        case QCameraExposure::ExposureAuto:
            return true; // Always supported
        case QCameraExposure::ExposureNight:
            if(supportedModes & CCamera::EExposureNight)
                return true;
            else
                return false;
        case QCameraExposure::ExposureBacklight:
            if(supportedModes & CCamera::EExposureBacklight)
                return true;
            else
                return false;
        case QCameraExposure::ExposureSports:
            if(supportedModes & CCamera::EExposureSport)
                return true;
            else
                return false;
        case QCameraExposure::ExposureSnow:
            if(supportedModes & CCamera::EExposureSnow)
                return true;
            else
                return false;
        case QCameraExposure::ExposureBeach:
            if(supportedModes & CCamera::EExposureBeach)
                return true;
            else
                return false;

        default:
            return false;
    }
}

void S60ImageCaptureSession::setExposureMode(QCameraExposure::ExposureMode mode)
{
    TRAPD(err, doSetExposureModeL(mode));
    setError(err, tr("Failed to set exposure mode."));
}

void S60ImageCaptureSession::doSetExposureModeL( QCameraExposure::ExposureMode mode)
{
    if (m_cameraEngine && m_cameraEngine->Camera()) {
        CCamera *camera = m_cameraEngine->Camera();
        switch(mode) {
            case QCameraExposure::ExposureManual:
                camera->SetExposureL(CCamera::EExposureManual);
                break;
            case QCameraExposure::ExposureAuto:
                camera->SetExposureL(CCamera::EExposureAuto);
                break;
            case QCameraExposure::ExposureNight:
                camera->SetExposureL(CCamera::EExposureNight);
                break;
            case QCameraExposure::ExposureBacklight:
                camera->SetExposureL(CCamera::EExposureBacklight);
                break;
            case QCameraExposure::ExposureSports:
                camera->SetExposureL(CCamera::EExposureSport);
                break;
            case QCameraExposure::ExposureSnow:
                camera->SetExposureL(CCamera::EExposureSnow);
                break;
            case QCameraExposure::ExposureBeach:
                camera->SetExposureL(CCamera::EExposureBeach);
                break;
            case QCameraExposure::ExposureLargeAperture:
            case QCameraExposure::ExposureSmallAperture:
                break;
            case QCameraExposure::ExposurePortrait:
            case QCameraExposure::ExposureSpotlight:
            default:
                setError(KErrNotSupported, tr("Requested exposure mode is not suported"));
                break;
        }
    } else {
        setError(KErrNotReady, tr("Unexpected camera error."));
    }
}

int S60ImageCaptureSession::contrast() const
{
    if (m_cameraEngine && m_cameraEngine->Camera()) {
        return m_cameraEngine->Camera()->Contrast();
    } else {
        return 0;
    }
}

void S60ImageCaptureSession::setContrast(int value)
{
    if (m_cameraEngine && m_cameraEngine->Camera()) {
        TRAPD(err, m_cameraEngine->Camera()->SetContrastL(value));
        setError(err, tr("Failed to set contrast."));
    } else {
        setError(KErrNotReady, tr("Unexpected camera error."));
    }
}

int S60ImageCaptureSession::brightness() const
{
    if (m_cameraEngine && m_cameraEngine->Camera()) {
        return m_cameraEngine->Camera()->Brightness();
    } else {
        return 0;
    }
}

void S60ImageCaptureSession::setBrightness(int value)
{
    if (m_cameraEngine && m_cameraEngine->Camera()) {
        TRAPD(err, m_cameraEngine->Camera()->SetBrightnessL(value));
        setError(err, tr("Failed to set brightness."));
    } else {
        setError(KErrNotReady, tr("Unexpected camera error."));
    }
}

 QCameraImageProcessing::WhiteBalanceMode S60ImageCaptureSession::whiteBalanceMode()
{
    if (m_cameraEngine && m_cameraEngine->Camera()) {
        CCamera::TWhiteBalance mode = m_cameraEngine->Camera()->WhiteBalance();
        switch(mode) {
            case CCamera::EWBAuto:
                return  QCameraImageProcessing::WhiteBalanceAuto;
            case CCamera::EWBDaylight:
                return  QCameraImageProcessing::WhiteBalanceSunlight;
            case CCamera::EWBCloudy:
                return  QCameraImageProcessing::WhiteBalanceCloudy;
            case CCamera::EWBTungsten:
                return  QCameraImageProcessing::WhiteBalanceTungsten;
            case CCamera::EWBFluorescent:
                return  QCameraImageProcessing::WhiteBalanceFluorescent;
            case CCamera::EWBFlash:
                return  QCameraImageProcessing::WhiteBalanceFlash;
            case CCamera::EWBBeach:
                return  QCameraImageProcessing::WhiteBalanceSunset;
            case CCamera::EWBManual:
                return  QCameraImageProcessing::WhiteBalanceManual;
            case CCamera::EWBShade:
                return  QCameraImageProcessing::WhiteBalanceShade;

            default:
                return  QCameraImageProcessing::WhiteBalanceAuto;
        }
    }

    return  QCameraImageProcessing::WhiteBalanceAuto;
}

void S60ImageCaptureSession::setWhiteBalanceMode( QCameraImageProcessing::WhiteBalanceMode mode)
{
    TRAPD(err, doSetWhiteBalanceModeL(mode));
    setError(err, tr("Failed to set white balance mode."));
}

void S60ImageCaptureSession::doSetWhiteBalanceModeL( QCameraImageProcessing::WhiteBalanceMode mode)
{
    if (m_cameraEngine && m_cameraEngine->Camera()) {
        CCamera* camera = m_cameraEngine->Camera();
        switch(mode) {
            case  QCameraImageProcessing::WhiteBalanceAuto:
                camera->SetWhiteBalanceL(CCamera::EWBAuto);
                break;
            case  QCameraImageProcessing::WhiteBalanceSunlight:
                camera->SetWhiteBalanceL(CCamera::EWBDaylight);
                break;
            case  QCameraImageProcessing::WhiteBalanceCloudy:
                camera->SetWhiteBalanceL(CCamera::EWBCloudy);
                break;
            case  QCameraImageProcessing::WhiteBalanceTungsten:
                camera->SetWhiteBalanceL(CCamera::EWBTungsten);
                break;
            case  QCameraImageProcessing::WhiteBalanceFluorescent:
                camera->SetWhiteBalanceL(CCamera::EWBFluorescent);
                break;
            case  QCameraImageProcessing::WhiteBalanceFlash:
                camera->SetWhiteBalanceL(CCamera::EWBFlash);
                break;
            case  QCameraImageProcessing::WhiteBalanceSunset:
                camera->SetWhiteBalanceL(CCamera::EWBBeach);
                break;
            case  QCameraImageProcessing::WhiteBalanceManual:
                camera->SetWhiteBalanceL(CCamera::EWBManual);
                break;
            case  QCameraImageProcessing::WhiteBalanceShade:
                camera->SetWhiteBalanceL(CCamera::EWBShade);
                break;

            default:
                setError(KErrNotSupported, tr("Requested white balance mode is not suported"));
                break;
        }
    } else {
        setError(KErrNotReady, tr("Unexpected camera error."));
    }
}

bool S60ImageCaptureSession::isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const
{
    if (m_cameraEngine) {
        TInt supportedModes = m_cameraInfo->iWhiteBalanceModesSupported;
        switch (mode) {
            case QCameraImageProcessing::WhiteBalanceManual:
                if (supportedModes & CCamera::EWBManual)
                    return true;
                else
                    return false;
            case QCameraImageProcessing::WhiteBalanceAuto:
                if (supportedModes & CCamera::EWBAuto)
                    return true;
                else
                    return false;
            case QCameraImageProcessing::WhiteBalanceSunlight:
                if (supportedModes & CCamera::EWBDaylight)
                    return true;
                else
                    return false;
            case QCameraImageProcessing::WhiteBalanceCloudy:
                if (supportedModes & CCamera::EWBCloudy)
                    return true;
                else
                    return false;
            case QCameraImageProcessing::WhiteBalanceShade:
                if (supportedModes & CCamera::EWBShade)
                    return true;
                else
                    return false;
            case QCameraImageProcessing::WhiteBalanceTungsten:
                if (supportedModes & CCamera::EWBTungsten)
                    return true;
                else
                    return false;
            case QCameraImageProcessing::WhiteBalanceFluorescent:
                if (supportedModes & CCamera::EWBFluorescent)
                    return true;
                else
                    return false;
            case QCameraImageProcessing::WhiteBalanceIncandescent: // Not available in Symbian
                    return false;
            case QCameraImageProcessing::WhiteBalanceFlash:
                if (supportedModes & CCamera::EWBFlash)
                    return true;
                else
                    return false;
            case QCameraImageProcessing::WhiteBalanceSunset:
                if (supportedModes & CCamera::EWBBeach)
                    return true;
                else
                    return false;

            default:
                return false;
        }
    }

    return false;
}

/*
 * ====================
 * S60 3.1 AutoFocosing
 * ====================
 */
bool S60ImageCaptureSession::isFocusSupported() const
{
    return m_cameraEngine->IsAutoFocusSupported();
}

void S60ImageCaptureSession::startFocus()
{
    if (m_cameraEngine) {
        TRAPD(err, m_cameraEngine->StartFocusL());
        setError(err, tr("Failed to start focusing."));
    }
    else
        setError(KErrNotReady, tr("Unexpected camera error."));
}

void S60ImageCaptureSession::cancelFocus()
{
    if (m_cameraEngine) {
        TRAPD(err, m_cameraEngine->FocusCancel());
        setError(err, tr("Failed to cancel focusing."));
    }
    else
        setError(KErrNotReady, tr("Unexpected camera error."));
}

void S60ImageCaptureSession::handleImageDecoded(int error)
{
    // Delete unneeded objects
    if (m_imageDecoder) {
        delete m_imageDecoder;
        m_imageDecoder = 0;
    }
    if (m_fileSystemAccess) {
        m_fileSystemAccess->Close();
        delete m_fileSystemAccess;
        m_fileSystemAccess = 0;
    }

    // Check status of decoding
    if (error != KErrNone) {
        if (m_previewBitmap) {
            m_previewBitmap->Reset();
            delete m_previewBitmap;
            m_previewBitmap = 0;
        }
        releaseImageBuffer();
        if (m_previewInWaitLoop) {
            CActiveScheduler::Stop(); // Notify to continue execution of next Preview Image generation
            m_previewInWaitLoop = false; // Reset
        }
        setError(error, tr("Preview creation failed."));
        return;
    }

    m_previewDecodingOngoing = false;

    QPixmap prevPixmap = QPixmap::fromSymbianCFbsBitmap(m_previewBitmap);
    QImage preview = prevPixmap.toImage();

    if (m_previewBitmap) {
        m_previewBitmap->Reset();
        delete m_previewBitmap;
        m_previewBitmap = 0;
    }

    QT_TRYCATCH_LEAVING( emit imageCaptured(m_currentImageId, preview) );

    // Release image resources (if not already done)
    releaseImageBuffer();

    if (m_previewInWaitLoop) {
        CActiveScheduler::Stop(); // Notify to continue execution of next Preview Image generation
        m_previewInWaitLoop = false; // Reset
    }
}

void S60ImageCaptureSession::handleImageEncoded(int error)
{
    // Check status of encoding
    if (error != KErrNone) {
        releaseImageBuffer();
        if (m_previewInWaitLoop) {
            CActiveScheduler::Stop(); // Notify to continue execution of next Preview Image generation
            m_previewInWaitLoop = false; // Reset
        }
        setError(error, tr("Saving captured image to file failed."));
        return;
    } else {
        QT_TRYCATCH_LEAVING( emit imageSaved(m_currentImageId, m_stillCaptureFileName) );
    }

    if (m_imageEncoder) {
        delete m_imageEncoder;
        m_imageEncoder = 0;
    }

#ifndef ECAM_PREVIEW_API
    // Start preview generation
    TInt previewError = KErrNone;
    TFileName fileName = convertImagePath();
    TRAP(previewError, m_imageDecoder = S60ImageCaptureDecoder::FileNewL(this, m_fileSystemAccess, &fileName));
    if (previewError) {
        setError(previewError, tr("Failed to create preview image."));
        return;
    }

    // Set proper Preview Size
    TSize scaledSize((m_captureSize.width() / KSnapshotDownScaleFactor), (m_captureSize.height() / KSnapshotDownScaleFactor));
    if (scaledSize.iWidth < KSnapshotMinWidth || scaledSize.iHeight < KSnapshotMinHeight)
        scaledSize.SetSize((m_captureSize.width() / (KSnapshotDownScaleFactor/2)), (m_captureSize.height() / (KSnapshotDownScaleFactor/2)));
    if (scaledSize.iWidth < KSnapshotMinWidth || scaledSize.iHeight < KSnapshotMinHeight)
        scaledSize.SetSize((m_captureSize.width() / (KSnapshotDownScaleFactor/4)), (m_captureSize.height() / (KSnapshotDownScaleFactor/4)));
    if (scaledSize.iWidth < KSnapshotMinWidth || scaledSize.iHeight < KSnapshotMinHeight)
        scaledSize.SetSize(m_captureSize.width(), m_captureSize.height());

    TFrameInfo *info = m_imageDecoder->frameInfo();
    if (!info) {
        setError(KErrGeneral, tr("Preview image creation failed."));
        return;
    }

    m_previewBitmap = new CFbsBitmap;
    if (!m_previewBitmap) {
        setError(KErrNoMemory, tr("Failed to create preview image."));
        return;
    }
    previewError = m_previewBitmap->Create(scaledSize, info->iFrameDisplayMode);
    if (previewError) {
        setError(previewError, tr("Preview creation failed."));
        return;
    }

    // Jpeg decoding completes in handleImageDecoded()
    m_imageDecoder->decode(m_previewBitmap);
#endif // ECAM_PREVIEW_API

    // Buffer can be released since Preview is created from file
    releaseImageBuffer();

    // Inform that we can continue taking more pictures
    QT_TRYCATCH_LEAVING( emit readyForCaptureChanged(true) );
}

#ifdef ECAM_PREVIEW_API
void S60ImageCaptureSession::MceoPreviewReady(CFbsBitmap& aPreview)
{
    QPixmap previewPixmap = QPixmap::fromSymbianCFbsBitmap(&aPreview);
    QImage preview = previewPixmap.toImage();

    // Notify preview availability
    emit imageCaptured(m_currentImageId, preview);
}
#endif // ECAM_PREVIEW_API

// End of file


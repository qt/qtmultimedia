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

#ifndef S60CAMERACONSTANTS_H
#define S60CAMERACONSTANTS_H

//=============================================================================

// GENERAL SETTINGS

#define KDefaultCameraDevice            0
#define KECamCameraPriority             0
#define KInactivityTimerTimeout         30000   // msec
#define KSymbianFineResolutionFactor    100.0
#define KDefaultOpticalZoom             1.0
#define KDefaultDigitalZoom             1.0
#define KSmoothZoomStep                 1
#define KDefaultFocusMode               QCameraFocus::AutoFocus

#define KDefaultViewfinderSize          QSize(320,240)
#define KDefaultSizePreview_Normal      TSize(640,480)
#define KDefaultSizePreview_Wide        TSize(640,360)
#define KDefaultSizePreview_CIF         TSize(352,288)
#define KDefaultSizePreview_PAL         TSize(640,512)
#define KDefaultSizePreview_NTSC        TSize(640,426)
#define KDefaultFormatPreview           CCamera::EFormatFbsBitmapColor16MU
#define KViewfinderFrameRate            30
#define KMaxVFErrorsSignalled           3

//=============================================================================

// IMAGE SETTINGS

#define KDefaultImagePath               QLatin1String("c:\\Data\\Images")
#define KDefaultImageFileName           QLatin1String("image.jpg")
#define KDefaultImageCodec              QLatin1String("image/jpeg")
#define KDefaultImageFormatPrimaryCam   CCamera::EFormatExif
#ifdef SYMBIAN_3_PLATFORM
#define KDefaultImageFormatSecondaryCam CCamera::EFormatExif
#define KDefaultImageResolution         QSize(3264, 2448)
#else // Pre-Symbian3 Platforms
#define KDefaultImageFormatSecondaryCam CCamera::EFormatFbsBitmapColor64K
#define KDefaultImageResolution         QSize(2048, 1536)
#endif // SYMBIAN_3_PLATFORM
#define KSymbianImageQualityCoefficient 25
// This must be divisible by 4 and creater or equal to 8
#define KSnapshotDownScaleFactor        8
#define KSnapshotMinWidth               640
#define KSnapshotMinHeight              360
#define KJpegQualityVeryLow             40
#define KJpegQualityLow                 50
#define KJpegQualityNormal              75
#define KJpegQualityHigh                85
#define KJpegQualityVeryHigh            95
#define KDefaultImageQuality            KJpegQualityHigh

//=============================================================================

// VIDEO SETTINGS

// ================
// General settings
// ================

// Dummy file name to execute CVideoRecorderUtility::OpenFileL() without
// knowing the actual outputLocation. This is needed to be able to query/set
// supported video settings.
_LIT(KDummyVideoFile, "c:\\data\\temp");

// Default container MIME type
#define KMimeTypeDefaultContainer   QLatin1String("video/mp4")
#define KDefaultVideoPath           QLatin1String("c:\\Data\\Videos")
#define KDefaultVideoFileName       QLatin1String("video.mp4")
#define KDurationChangedInterval    1000 // 1 second

// ==============
// Audio Settings
// ==============

// Default audio codec MIME type
#define KMimeTypeDefaultAudioCodec  QLatin1String("audio/aac")

// Default audio settings for video recording
#define KDefaultChannelCount  -1 // Not Supported on Symbian
#define KDefaultBitRate       32000 // 32kbps
#define KDefaultSampleRate    -1 // Not Supported on Symbian

// ==============
// Video Settings
// ==============

// Default video codec MIME type
#ifdef SYMBIAN_3_PLATFORM
    // H.264: BaselineProfile Level 3.1, Max resolution: 1280x720
    #define KMimeTypeDefaultVideoCodec QLatin1String("video/H264; profile-level-id=42801F")
#else
    // MPEG-4: Simple Profile, Level 4, Max resolution: 640x480
    #define KMimeTypeDefaultVideoCodec QLatin1String("video/mp4v-es; profile-level-id=4")
#endif

// Maximum resolutions for encoder MIME Types
// H.263
#define KResH263                   QSize(176,144);
#define KResH263_Profile0          QSize(176,144);
#define KResH263_Profile0_Level10  QSize(176,144);
#define KResH263_Profile0_Level20  QSize(352,288);
#define KResH263_Profile0_Level30  QSize(352,288);
#define KResH263_Profile0_Level40  QSize(352,288);
#define KResH263_Profile0_Level45  QSize(176,144);
#define KResH263_Profile0_Level50  QSize(352,288);
#define KResH263_Profile3          QSize(176,144);
// MPEG-4
#define KResMPEG4          QSize(176,144);
#define KResMPEG4_PLID_1   QSize(176,144);
#define KResMPEG4_PLID_2   QSize(352,288);
#define KResMPEG4_PLID_3   QSize(352,288);
#define KResMPEG4_PLID_4   QSize(640,480);
#define KResMPEG4_PLID_5   QSize(720,576);
#define KResMPEG4_PLID_6   QSize(1280,720);
#define KResMPEG4_PLID_8   QSize(176,144);
#define KResMPEG4_PLID_9   QSize(176,144);
// H.264 (Baseline Profile, same resolutions apply to Main and High Profile)
#define KResH264               QSize(176,144);
#define KResH264_PLID_42800A   QSize(176,144);
#define KResH264_PLID_42900B   QSize(176,144);
#define KResH264_PLID_42800B   QSize(352,288);
#define KResH264_PLID_42800C   QSize(352,288);
#define KResH264_PLID_42800D   QSize(352,288);
#define KResH264_PLID_428014   QSize(352,288);
#define KResH264_PLID_428015   QSize(352,288);
#define KResH264_PLID_428016   QSize(640,480);
#define KResH264_PLID_42801E   QSize(640,480);
#define KResH264_PLID_42801F   QSize(1280,720);
#define KResH264_PLID_428020   QSize(1280,720);
#define KResH264_PLID_428028   QSize(1920,1080);

// Maximum framerates for encoder MIME Types
// H.263
#define KFrR_H263                   qreal(15);
#define KFrR_H263_Profile0          qreal(15);
#define KFrR_H263_Profile0_Level10  qreal(15);
#define KFrR_H263_Profile0_Level20  qreal(15);
#define KFrR_H263_Profile0_Level30  qreal(30);
#define KFrR_H263_Profile0_Level40  qreal(30);
#define KFrR_H263_Profile0_Level45  qreal(15);
#define KFrR_H263_Profile0_Level50  qreal(15);
#define KFrR_H263_Profile3          qreal(15);
// MPEG-4
#define KFrR_MPEG4          qreal(15);
#define KFrR_MPEG4_PLID_1   qreal(15);
#define KFrR_MPEG4_PLID_2   qreal(15);
#define KFrR_MPEG4_PLID_3   qreal(30);
// This is a workaround for a known platform bug
#if (defined(S60_31_PLATFORM) | defined(S60_32_PLATFORM))
#define KFrR_MPEG4_PLID_4   qreal(15);
#else // All other platforms
#define KFrR_MPEG4_PLID_4   qreal(30);
#endif // S60 3.1 or 3.2
#define KFrR_MPEG4_PLID_5   qreal(30);
#define KFrR_MPEG4_PLID_6   qreal(30);
#define KFrR_MPEG4_PLID_8   qreal(15);
#define KFrR_MPEG4_PLID_9   qreal(15);
// H.264 (Baseline Profile, same framerates apply to Main and High Profile)
#define KFrR_H264               qreal(15);
#define KFrR_H264_PLID_42800A   qreal(15);
#define KFrR_H264_PLID_42900B   qreal(15);
#define KFrR_H264_PLID_42800B   qreal(7.5);
#define KFrR_H264_PLID_42800C   qreal(15);
#define KFrR_H264_PLID_42800D   qreal(30);
#define KFrR_H264_PLID_428014   qreal(30);
#define KFrR_H264_PLID_428015   qreal(50);
#define KFrR_H264_PLID_428016   qreal(16.9);
#define KFrR_H264_PLID_42801E   qreal(33.8);
#define KFrR_H264_PLID_42801F   qreal(30);
#define KFrR_H264_PLID_428020   qreal(60);
#define KFrR_H264_PLID_428028   qreal(30);

// Maximum bitrates for encoder MIME Types
// H.263
#define KBiR_H263                   int(64000);
#define KBiR_H263_Profile0          int(64000);
#define KBiR_H263_Profile0_Level10  int(64000);
#define KBiR_H263_Profile0_Level20  int(128000);
#define KBiR_H263_Profile0_Level30  int(384000);
#define KBiR_H263_Profile0_Level40  int(2048000);
#define KBiR_H263_Profile0_Level45  int(128000);
#define KBiR_H263_Profile0_Level50  int(4096000);
#define KBiR_H263_Profile3          int(64000);
// MPEG-4
#define KBiR_MPEG4          int(64000);
#define KBiR_MPEG4_PLID_1   int(64000);
#define KBiR_MPEG4_PLID_2   int(128000);
#define KBiR_MPEG4_PLID_3   int(384000);
// This is a workaround for a known platform bug
#if (defined(S60_31_PLATFORM) | defined(S60_32_PLATFORM))
#define KBiR_MPEG4_PLID_4   int(2000000);
#else // All other platforms
#define KBiR_MPEG4_PLID_4   int(4000000);
#endif // S60 3.1 or 3.2
#define KBiR_MPEG4_PLID_5   int(8000000);
#define KBiR_MPEG4_PLID_6   int(12000000);
#define KBiR_MPEG4_PLID_8   int(64000);
#define KBiR_MPEG4_PLID_9   int(128000);
// H.264 (Baseline Profile, same bitrates apply to Main and High Profile)
#define KBiR_H264               int(64000);
#define KBiR_H264_PLID_42800A   int(64000);
#define KBiR_H264_PLID_42900B   int(128000);
#define KBiR_H264_PLID_42800B   int(192000);
#define KBiR_H264_PLID_42800C   int(384000);
#define KBiR_H264_PLID_42800D   int(768000);
#define KBiR_H264_PLID_428014   int(2000000);
#define KBiR_H264_PLID_428015   int(4000000);
#define KBiR_H264_PLID_428016   int(4000000);
#define KBiR_H264_PLID_42801E   int(10000000);
#define KBiR_H264_PLID_42801F   int(14000000);
#define KBiR_H264_PLID_428020   int(20000000);
#define KBiR_H264_PLID_428028   int(20000000);

#endif // S60CAMERACONSTANTS_H

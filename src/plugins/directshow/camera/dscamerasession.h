/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef DSCAMERASESSION_H
#define DSCAMERASESSION_H

#include <QtCore/qobject.h>
#include <QTime>
#include <QUrl>
#include <QMutex>

#include <qcamera.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qabstractvideosurface.h>
#include <QtMultimedia/qvideosurfaceformat.h>
#include <private/qmediastoragelocation_p.h>

#include <tchar.h>
#include <dshow.h>
#include <objbase.h>
#include <initguid.h>
#ifdef Q_CC_MSVC
#  pragma comment(lib, "strmiids.lib")
#  pragma comment(lib, "ole32.lib")
#endif // Q_CC_MSVC
#include <windows.h>

#ifdef Q_CC_MSVC
#  pragma include_alias("dxtrans.h","qedit.h")
#endif // Q_CC_MSVC
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__

struct ICaptureGraphBuilder2;
struct ISampleGrabber;

QT_BEGIN_NAMESPACE

class SampleGrabberCallbackPrivate;

class DSCameraSession : public QObject
{
    Q_OBJECT
public:
    DSCameraSession(QObject *parent = 0);
    ~DSCameraSession();

    QCamera::Status status() const { return m_status; }

    void setDevice(const QString &device);

    bool load();
    bool unload();
    bool startPreview();
    bool stopPreview();

    bool isReadyForCapture();
    int captureImage(const QString &fileName);

    void setSurface(QAbstractVideoSurface* surface);

    QCameraViewfinderSettings viewfinderSettings() const;
    void setViewfinderSettings(const QCameraViewfinderSettings &settings);

    QList<QCameraViewfinderSettings> supportedViewfinderSettings() const
    { return m_supportedViewfinderSettings; }

Q_SIGNALS:
    void statusChanged(QCamera::Status);
    void imageExposed(int id);
    void imageCaptured(int id, const QImage &preview);
    void imageSaved(int id, const QString &fileName);
    void readyForCaptureChanged(bool);
    void captureError(int id, int error, const QString &errorString);

private Q_SLOTS:
    void presentFrame();
    void updateReadyForCapture();

private:
    void setStatus(QCamera::Status status);

    void onFrameAvailable(const char *frameData, long len);
    void saveCapturedImage(int id, const QImage &image, const QString &path);

    bool createFilterGraph();
    bool connectGraph();
    void disconnectGraph();
    void updateSourceCapabilities();
    bool configurePreviewFormat();

    QMutex m_presentMutex;
    QMutex m_captureMutex;

    // Capture Graph
    ICaptureGraphBuilder2* m_graphBuilder;
    IGraphBuilder* m_filterGraph;

    // Source (camera)
    QString m_sourceDeviceName;
    IBaseFilter* m_sourceFilter;
    bool m_needsHorizontalMirroring;
    QList<AM_MEDIA_TYPE> m_supportedFormats;
    QList<QCameraViewfinderSettings> m_supportedViewfinderSettings;
    AM_MEDIA_TYPE m_sourceFormat;

    // Preview
    IBaseFilter *m_previewFilter;
    ISampleGrabber *m_previewSampleGrabber;
    IBaseFilter *m_nullRendererFilter;
    QVideoFrame m_currentFrame;
    bool m_previewStarted;
    QAbstractVideoSurface* m_surface;
    QVideoSurfaceFormat m_previewSurfaceFormat;
    QVideoFrame::PixelFormat m_previewPixelFormat;
    QSize m_previewSize;
    QCameraViewfinderSettings m_viewfinderSettings;
    QCameraViewfinderSettings m_actualViewfinderSettings;

    // Image capture
    QString m_imageCaptureFileName;
    QMediaStorageLocation m_fileNameGenerator;
    bool m_readyForCapture;
    int m_imageIdCounter;
    int m_currentImageId;
    QVideoFrame m_capturedFrame;

    // Internal state
    QCamera::Status m_status;

    friend class SampleGrabberCallbackPrivate;
};

QT_END_NAMESPACE


#endif

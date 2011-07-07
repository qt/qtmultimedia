/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "camerabinsession.h"
#include "camerabinrecorder.h"
#include "camerabincontainer.h"
#include "camerabinaudioencoder.h"
#include "camerabinvideoencoder.h"
#include "camerabinimageencoder.h"
#include "camerabinexposure.h"
#include "camerabinflash.h"
#include "camerabinfocus.h"
#include "camerabinimageprocessing.h"
#include "camerabinlocks.h"
#include "camerabincapturedestination.h"
#include "camerabincapturebufferformat.h"
#include "qgstreamerbushelper.h"
#include "qgstreamervideorendererinterface.h"
#include <qmediarecorder.h>
#include <gst/interfaces/photography.h>
#include <gst/gsttagsetter.h>
#include <gst/gstversion.h>

#include <QtCore/qdebug.h>
#include <QCoreApplication>
#include <QtCore/qmetaobject.h>
#include <QtGui/qdesktopservices.h>

#include <QtGui/qimage.h>

//#define CAMERABIN_DEBUG 1
#define ENUM_NAME(c,e,v) (c::staticMetaObject.enumerator(c::staticMetaObject.indexOfEnumerator(e)).valueToKey((v)))

#ifdef Q_WS_MAEMO_5
#define FILENAME_PROPERTY "filename"
#define MODE_PROPERTY "mode"
#define MUTE_PROPERTY "mute"
#define ZOOM_PROPERTY "zoom"
#define IMAGE_PP_PROPERTY "imagepp"
#define IMAGE_ENCODER_PROPERTY "imageenc"
#define VIDEO_PP_PROPERTY "videopp"
#define VIDEO_ENCODER_PROPERTY "videoenc"
#define AUDIO_ENCODER_PROPERTY "audioenc"
#define VIDEO_MUXER_PROPERTY "videomux"
#define VIEWFINDER_SINK_PROPERTY "vfsink"
#define VIDEO_SOURCE_PROPERTY "videosrc"
#define AUDIO_SOURCE_PROPERTY "audiosrc"
#define VIDEO_SOURCE_CAPS_PROPERTY "inputcaps"
#define FILTER_CAPS_PROPERTY "filter-caps"
#define PREVIEW_CAPS_PROPERTY "preview-caps"

#define IMAGE_DONE_SIGNAL "img-done"
#define CAPTURE_START "user-start"
#define CAPTURE_STOP "user-stop"
#define CAPTURE_PAUSE "user-pause"
#define SET_VIDEO_RESOLUTION_FPS "user-res-fps"
#define SET_IMAGE_RESOLUTION "user-image-res"

#else

#define FILENAME_PROPERTY "filename"
#define MODE_PROPERTY "mode"
#define MUTE_PROPERTY "mute"
#define ZOOM_PROPERTY "zoom"
#define IMAGE_PP_PROPERTY "image-post-processing"
#define IMAGE_ENCODER_PROPERTY "image-encoder"
#define VIDEO_PP_PROPERTY "video-post-processing"
#define VIDEO_ENCODER_PROPERTY "video-encoder"
#define AUDIO_ENCODER_PROPERTY "audio-encoder"
#define VIDEO_MUXER_PROPERTY "video-muxer"
#define VIEWFINDER_SINK_PROPERTY "viewfinder-sink"
#define VIDEO_SOURCE_PROPERTY "video-source"
#define AUDIO_SOURCE_PROPERTY "audio-source"
#define VIDEO_SOURCE_CAPS_PROPERTY "video-source-caps"
#define FILTER_CAPS_PROPERTY "filter-caps"
#define PREVIEW_CAPS_PROPERTY "preview-caps"

#define IMAGE_DONE_SIGNAL "image-done"
#define CAPTURE_START "capture-start"
#define CAPTURE_STOP "capture-stop"
#define CAPTURE_PAUSE "capture-pause"
#define SET_VIDEO_RESOLUTION_FPS "set-video-resolution-fps"
#define SET_IMAGE_RESOLUTION "set-image-resolution"
#endif

#define CAMERABIN_IMAGE_MODE 0
#define CAMERABIN_VIDEO_MODE 1

#define gstRef(element) { gst_object_ref(GST_OBJECT(element)); gst_object_sink(GST_OBJECT(element)); }
#define gstUnref(element) { if (element) { gst_object_unref(GST_OBJECT(element)); element = 0; } }

#define PREVIEW_CAPS_4_3 \
    "video/x-raw-rgb, width = (int) 640, height = (int) 480"

#define VIEWFINDER_RESOLUTION_4x3 QSize(640, 480)
#define VIEWFINDER_RESOLUTION_3x2 QSize(720, 480)
#define VIEWFINDER_RESOLUTION_16x9 QSize(800, 450)

//using GST_STATE_READY for QCamera::LoadedState
//doesn't work reliably at least with some webcams.
#if defined(Q_WS_MAEMO_5) || defined(Q_WS_MAEMO_6)
#define USE_READY_STATE_ON_LOADED
#endif

CameraBinSession::CameraBinSession(QObject *parent)
    :QObject(parent),
     m_state(QCamera::UnloadedState),
     m_pendingState(QCamera::UnloadedState),
     m_recordingActive(false),
     m_pendingResolutionUpdate(false),
     m_muted(false),
     m_busy(false),
     m_captureMode(QCamera::CaptureStillImage),
     m_audioInputFactory(0),
     m_videoInputFactory(0),
     m_viewfinder(0),
     m_viewfinderInterface(0),
     m_pipeline(0),
     m_videoSrc(0),
     m_viewfinderElement(0),
     m_viewfinderHasChanged(true),
     m_videoInputHasChanged(true),
     m_sourceCaps(0),
     m_audioSrc(0),
     m_audioConvert(0),
     m_capsFilter(0),
     m_fileSink(0),
     m_audioEncoder(0),
     m_muxer(0)
{
    m_pipeline = gst_element_factory_make("camerabin", "camerabin");
    g_signal_connect(G_OBJECT(m_pipeline), "notify::idle", G_CALLBACK(updateBusyStatus), this);

    gstRef(m_pipeline);

    m_bus = gst_element_get_bus(m_pipeline);

    m_busHelper = new QGstreamerBusHelper(m_bus, this);
    m_busHelper->installSyncEventFilter(this);
    connect(m_busHelper, SIGNAL(message(QGstreamerMessage)), SLOT(handleBusMessage(QGstreamerMessage)));
    m_audioEncodeControl = new CameraBinAudioEncoder(this);
    m_videoEncodeControl = new CameraBinVideoEncoder(this);
    m_imageEncodeControl = new CameraBinImageEncoder(this);
    m_recorderControl = new CameraBinRecorder(this);
    m_mediaContainerControl = new CameraBinContainer(this);
    m_cameraExposureControl = new CameraBinExposure(this);
    m_cameraFlashControl = new CameraBinFlash(this);
    m_cameraFocusControl = new CameraBinFocus(this);
    m_imageProcessingControl = new CameraBinImageProcessing(this);
    m_cameraLocksControl = new CameraBinLocks(this);
    m_captureDestinationControl = new CameraBinCaptureDestination(this);
    m_captureBufferFormatControl = new CameraBinCaptureBufferFormat(this);
}

CameraBinSession::~CameraBinSession()
{
    if (m_pipeline) {
        if (m_viewfinderInterface)
            m_viewfinderInterface->stopRenderer();

        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_element_get_state(m_pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
        gstUnref(m_pipeline);
        gstUnref(m_viewfinderElement);
    }
}

GstPhotography *CameraBinSession::photography()
{
    if (GST_IS_PHOTOGRAPHY(m_pipeline)) {
        return GST_PHOTOGRAPHY(m_pipeline);
    }

    if (!m_videoSrc) {
        m_videoSrc = buildVideoSrc();

        if (m_videoSrc)
            g_object_set(m_pipeline, VIDEO_SOURCE_PROPERTY, m_videoSrc, NULL);
        else
            g_object_get(m_pipeline, VIDEO_SOURCE_PROPERTY, &m_videoSrc, NULL);

        updateVideoSourceCaps();
        m_videoInputHasChanged = false;
    }

    if (m_videoSrc && GST_IS_PHOTOGRAPHY(m_videoSrc))
        return GST_PHOTOGRAPHY(m_videoSrc);

    return 0;
}

CameraBinSession::CameraRole CameraBinSession::cameraRole() const
{
#ifdef Q_WS_MAEMO_5
    return m_inputDevice == QLatin1String("/dev/video1") ?
                FrontCamera : BackCamera;
#endif

    return BackCamera;
}

bool CameraBinSession::setupCameraBin()
{
    if (m_captureMode == QCamera::CaptureStillImage) {
        g_object_set(m_pipeline, MODE_PROPERTY, CAMERABIN_IMAGE_MODE, NULL);
    }

    if (m_captureMode == QCamera::CaptureVideo) {
        g_object_set(m_pipeline, MODE_PROPERTY, CAMERABIN_VIDEO_MODE, NULL);

        if (!m_recorderControl->findCodecs())
            return false;

        g_object_set(m_pipeline, VIDEO_ENCODER_PROPERTY, m_videoEncodeControl->createEncoder(), NULL);
        g_object_set(m_pipeline, AUDIO_ENCODER_PROPERTY, m_audioEncodeControl->createEncoder(), NULL);
        g_object_set(m_pipeline, VIDEO_MUXER_PROPERTY,
                     gst_element_factory_make(m_mediaContainerControl->formatElementName().constData(), NULL), NULL);
    }    

    if (m_videoInputHasChanged) {
        m_videoSrc = buildVideoSrc();

        if (m_videoSrc)
            g_object_set(m_pipeline, VIDEO_SOURCE_PROPERTY, m_videoSrc, NULL);
        else
            g_object_get(m_pipeline, VIDEO_SOURCE_PROPERTY, &m_videoSrc, NULL);

        updateVideoSourceCaps();
        m_videoInputHasChanged = false;
    }


    if (m_viewfinderHasChanged) {
        if (m_viewfinderElement)
            gst_object_unref(GST_OBJECT(m_viewfinderElement));

        m_viewfinderElement = m_viewfinderInterface ? m_viewfinderInterface->videoSink() : 0;
#if CAMERABIN_DEBUG
        qDebug() << Q_FUNC_INFO << "Viewfinder changed, reconfigure.";
#endif
        m_viewfinderHasChanged = false;
        if (!m_viewfinderElement) {
            qWarning() << "Staring camera without viewfinder available";
            m_viewfinderElement = gst_element_factory_make("fakesink", NULL);
        }
        gst_object_ref(GST_OBJECT(m_viewfinderElement));
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        g_object_set(G_OBJECT(m_pipeline), VIEWFINDER_SINK_PROPERTY, m_viewfinderElement, NULL);
    }

    GstCaps *previewCaps = gst_caps_from_string(PREVIEW_CAPS_4_3);
    g_object_set(G_OBJECT(m_pipeline), PREVIEW_CAPS_PROPERTY, previewCaps, NULL);
    gst_caps_unref(previewCaps);

    return true;
}

void CameraBinSession::updateVideoSourceCaps()
{
    if (m_sourceCaps) {
        gst_caps_unref(m_sourceCaps);
        m_sourceCaps = 0;
    }

    g_object_get(G_OBJECT(m_pipeline), VIDEO_SOURCE_CAPS_PROPERTY, &m_sourceCaps, NULL);
}

void CameraBinSession::setupCaptureResolution()
{
    if (m_captureMode == QCamera::CaptureStillImage) {
        QSize resolution = m_imageEncodeControl->imageSettings().resolution();

        //by default select the maximum supported resolution
        if (resolution.isEmpty()) {
            updateVideoSourceCaps();
            bool continuous = false;
            QList<QSize> resolutions = supportedResolutions(qMakePair<int,int>(0,0),
                                                            &continuous,
                                                            QCamera::CaptureStillImage);
            if (!resolutions.isEmpty())
                resolution = resolutions.last();
        }

        QString previewCapsString = PREVIEW_CAPS_4_3;
        QSize viewfinderResolution = VIEWFINDER_RESOLUTION_4x3;

        if (!resolution.isEmpty()) {
#if CAMERABIN_DEBUG
            qDebug() << Q_FUNC_INFO << "set image resolution" << resolution;
#endif
            g_signal_emit_by_name(G_OBJECT(m_pipeline), SET_IMAGE_RESOLUTION, resolution.width(), resolution.height(), NULL);

            previewCapsString = QString("video/x-raw-rgb, width = (int) %1, height = (int) 480")
                    .arg(resolution.width()*480/resolution.height());

            if (!resolution.isEmpty()) {
                qreal aspectRatio = qreal(resolution.width()) / resolution.height();
                if (aspectRatio < 1.4)
                    viewfinderResolution = VIEWFINDER_RESOLUTION_4x3;
                else if (aspectRatio > 1.7)
                    viewfinderResolution = VIEWFINDER_RESOLUTION_16x9;
                else
                    viewfinderResolution = VIEWFINDER_RESOLUTION_3x2;
            }
        }

        GstCaps *previewCaps = gst_caps_from_string(previewCapsString.toLatin1());
        g_object_set(G_OBJECT(m_pipeline), PREVIEW_CAPS_PROPERTY, previewCaps, NULL);
        gst_caps_unref(previewCaps);

        //on low res cameras the viewfinder resolution should not be bigger
        //then capture resolution
        if (viewfinderResolution.width() > resolution.width())
            viewfinderResolution = resolution;

#if CAMERABIN_DEBUG
            qDebug() << Q_FUNC_INFO << "set viewfinder resolution" << viewfinderResolution;
#endif
        g_signal_emit_by_name(G_OBJECT(m_pipeline),
                              SET_VIDEO_RESOLUTION_FPS,
                              viewfinderResolution.width(),
                              viewfinderResolution.height(),
                              0, // maximum framerate
                              1, // framerate denom
                              NULL);
    }

    if (m_captureMode == QCamera::CaptureVideo) {
        QSize resolution = m_videoEncodeControl->videoSettings().resolution();
        qreal framerate = m_videoEncodeControl->videoSettings().frameRate();

        if (resolution.isEmpty()) {
            //select the hightest supported resolution

            updateVideoSourceCaps();
            bool continuous = false;
            QList<QSize> resolutions = supportedResolutions(qMakePair<int,int>(0,0),
                                                            &continuous,
                                                            QCamera::CaptureVideo);
            if (!resolutions.isEmpty())
                resolution = resolutions.last();
        }

        if (!resolution.isEmpty() || framerate > 0) {
#if CAMERABIN_DEBUG
            qDebug() << Q_FUNC_INFO << "set video resolution" << resolution;
#endif
            g_signal_emit_by_name(G_OBJECT(m_pipeline),
                                  SET_VIDEO_RESOLUTION_FPS,
                                  resolution.width(),
                                  resolution.height(),
                                  0, //framerate nom == max rate
                                  1, // framerate denom == max rate
                                  NULL);
        }
    }
}

GstElement *CameraBinSession::buildVideoSrc()
{
    GstElement *videoSrc = 0;
    if (m_videoInputFactory) {
        videoSrc = m_videoInputFactory->buildElement();
    } else {
        QList<QByteArray> candidates;
        candidates << "subdevsrc"
                   << "v4l2camsrc"
                   << "v4l2src"
                   << "autovideosrc";
        QByteArray sourceElementName;

        foreach(sourceElementName, candidates) {
            videoSrc = gst_element_factory_make(sourceElementName.constData(), "camera_source");
            if (videoSrc)
                break;
        }

        if (videoSrc && !m_inputDevice.isEmpty()) {
#if CAMERABIN_DEBUG
            qDebug() << "set camera device" << m_inputDevice;
#endif
            if (sourceElementName == "subdevsrc") {
                if (m_inputDevice == QLatin1String("secondary"))
                    g_object_set(G_OBJECT(videoSrc), "camera-device", 1, NULL);
                else
                    g_object_set(G_OBJECT(videoSrc), "camera-device", 0, NULL);
            } else {
                g_object_set(G_OBJECT(videoSrc), "device", m_inputDevice.toLocal8Bit().constData(), NULL);
            }
        }
    }

    return videoSrc;
}

void CameraBinSession::captureImage(int requestId, const QString &fileName)
{
    QString actualFileName = fileName;
    if (actualFileName.isEmpty())
        actualFileName = generateFileName("img_", defaultDir(QCamera::CaptureStillImage), "jpg");

    m_requestId = requestId;

    g_object_set(G_OBJECT(m_pipeline), FILENAME_PROPERTY, actualFileName.toLocal8Bit().constData(), NULL);

    g_signal_emit_by_name(G_OBJECT(m_pipeline), CAPTURE_START, NULL);

    m_imageFileName = actualFileName;
}

void CameraBinSession::setCaptureMode(QCamera::CaptureMode mode)
{
    m_captureMode = mode;

    switch (m_captureMode) {
    case QCamera::CaptureStillImage:
        g_object_set(m_pipeline, MODE_PROPERTY, CAMERABIN_IMAGE_MODE, NULL);
        break;
    case QCamera::CaptureVideo:
        g_object_set(m_pipeline, MODE_PROPERTY, CAMERABIN_VIDEO_MODE, NULL);
        break;
    }
}

QUrl CameraBinSession::outputLocation() const
{
    //return the location service wrote data to, not one set by user, it can be empty.
    return m_actualSink;
}

bool CameraBinSession::setOutputLocation(const QUrl& sink)
{
    m_sink = m_actualSink = sink;
    return true;
}

QDir CameraBinSession::defaultDir(QCamera::CaptureMode mode) const
{
    QStringList dirCandidates;

#if defined(Q_WS_MAEMO_5) || defined(Q_WS_MAEMO_6)
    dirCandidates << QLatin1String("/home/user/MyDocs/DCIM");
    dirCandidates << QLatin1String("/home/user/MyDocs/");
#endif

    if (mode == QCamera::CaptureVideo) {
        dirCandidates << QDesktopServices::storageLocation(QDesktopServices::MoviesLocation);
        dirCandidates << QDir::home().filePath("Documents/Video");
        dirCandidates << QDir::home().filePath("Documents/Videos");
    } else {
        dirCandidates << QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
        dirCandidates << QDir::home().filePath("Documents/Photo");
        dirCandidates << QDir::home().filePath("Documents/Photos");
        dirCandidates << QDir::home().filePath("Documents/photo");
        dirCandidates << QDir::home().filePath("Documents/photos");
        dirCandidates << QDir::home().filePath("Documents/Images");
    }

    dirCandidates << QDir::home().filePath("Documents");
    dirCandidates << QDir::home().filePath("My Documents");
    dirCandidates << QDir::homePath();
    dirCandidates << QDir::currentPath();
    dirCandidates << QDir::tempPath();

    foreach (const QString &path, dirCandidates) {
        if (QFileInfo(path).isWritable())
            return QDir(path);
    }

    return QDir();
}

QString CameraBinSession::generateFileName(const QString &prefix, const QDir &dir, const QString &ext) const
{
    int lastClip = 0;
    foreach(QString fileName, dir.entryList(QStringList() << QString("%1*.%2").arg(prefix).arg(ext))) {
        int imgNumber = fileName.mid(prefix.length(), fileName.size()-prefix.length()-ext.length()-1).toInt();
        lastClip = qMax(lastClip, imgNumber);
    }

    QString name = QString("%1%2.%3").arg(prefix)
                                     .arg(lastClip+1,
                                     4, //fieldWidth
                                     10,
                                     QLatin1Char('0'))
                                     .arg(ext);

    return dir.absoluteFilePath(name);
}

void CameraBinSession::setDevice(const QString &device)
{
    if (m_inputDevice != device) {
        m_inputDevice = device;
        m_videoInputHasChanged = true;
    }
}

void CameraBinSession::setAudioInput(QGstreamerElementFactory *audioInput)
{
    m_audioInputFactory = audioInput;
}

void CameraBinSession::setVideoInput(QGstreamerElementFactory *videoInput)
{
    m_videoInputFactory = videoInput;
    m_videoInputHasChanged = true;
}

bool CameraBinSession::isReady() const
{
    //it's possible to use QCamera without any viewfinder attached
    return !m_viewfinderInterface || m_viewfinderInterface->isReady();
}

void CameraBinSession::setViewfinder(QObject *viewfinder)
{
    if (m_viewfinderInterface)
        m_viewfinderInterface->stopRenderer();

    m_viewfinderInterface = qobject_cast<QGstreamerVideoRendererInterface*>(viewfinder);
    if (!m_viewfinderInterface)
        viewfinder = 0;

    if (m_viewfinder != viewfinder) {
        bool oldReady = isReady();

        if (m_viewfinder) {
            disconnect(m_viewfinder, SIGNAL(sinkChanged()),
                       this, SLOT(handleViewfinderChange()));
            disconnect(m_viewfinder, SIGNAL(readyChanged(bool)),
                       this, SIGNAL(readyChanged(bool)));
        }

        m_viewfinder = viewfinder;
        m_viewfinderHasChanged = true;

        if (m_viewfinder) {
            connect(m_viewfinder, SIGNAL(sinkChanged()),
                       this, SLOT(handleViewfinderChange()));
            connect(m_viewfinder, SIGNAL(readyChanged(bool)),
                    this, SIGNAL(readyChanged(bool)));
        }

        emit viewfinderChanged();
        if (oldReady != isReady())
            emit readyChanged(isReady());
    }
}

void CameraBinSession::handleViewfinderChange()
{
    //the viewfinder will be reloaded
    //shortly when the pipeline is started
    m_viewfinderHasChanged = true;
    emit viewfinderChanged();
}

QCamera::State CameraBinSession::state() const
{
    return m_state;
}

void CameraBinSession::setState(QCamera::State newState)
{
    if (newState == m_pendingState)
        return;

    m_pendingState = newState;

#if CAMERABIN_DEBUG
    qDebug() << Q_FUNC_INFO << ENUM_NAME(QCamera, "State", newState);
#endif

    switch (newState) {
    case QCamera::UnloadedState:
        if (m_recordingActive)
            stopVideoRecording();

        if (m_viewfinderInterface)
            m_viewfinderInterface->stopRenderer();

        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        m_state = newState;
        if (m_busy)
            emit busyChanged(m_busy = false);

        emit stateChanged(m_state);
        break;
    case QCamera::LoadedState:
        if (m_recordingActive)
            stopVideoRecording();

        if (m_videoInputHasChanged) {
            if (m_viewfinderInterface)
                m_viewfinderInterface->stopRenderer();

            gst_element_set_state(m_pipeline, GST_STATE_NULL);
            m_videoSrc = buildVideoSrc();
            g_object_set(m_pipeline, VIDEO_SOURCE_PROPERTY, m_videoSrc, NULL);
            updateVideoSourceCaps();
            m_videoInputHasChanged = false;
        }
#ifdef USE_READY_STATE_ON_LOADED
        gst_element_set_state(m_pipeline, GST_STATE_READY);
#else
        m_state = QCamera::LoadedState;
        if (m_viewfinderInterface)
            m_viewfinderInterface->stopRenderer();
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        emit stateChanged(m_state);
#endif
        break;
    case QCamera::ActiveState:
        if (setupCameraBin()) {
            GstState binState = GST_STATE_NULL;
            GstState pending = GST_STATE_NULL;
            gst_element_get_state(m_pipeline, &binState, &pending, 0);

            if (pending == GST_STATE_VOID_PENDING && binState == GST_STATE_READY) {
                m_pendingResolutionUpdate = false;
                setupCaptureResolution();
                gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
            } else {
                m_pendingResolutionUpdate = true;
                gst_element_set_state(m_pipeline, GST_STATE_READY);
            }
        }
    }
}

bool CameraBinSession::isBusy() const
{
    return m_busy;
}

void CameraBinSession::updateBusyStatus(GObject *o, GParamSpec *p, gpointer d)
{
    Q_UNUSED(p);
    CameraBinSession *session = reinterpret_cast<CameraBinSession *>(d);

    bool idle = false;
    g_object_get(o, "idle", &idle, NULL);
    bool busy = !idle;

    if (session->m_busy != busy) {
        session->m_busy = busy;
        QMetaObject::invokeMethod(session, "busyChanged",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, busy));
    }
}

qint64 CameraBinSession::duration() const
{
    GstFormat   format = GST_FORMAT_TIME;
    gint64      duration = 0;

    if ( m_pipeline && gst_element_query_position(m_pipeline, &format, &duration))
        return duration / 1000000;
    else
        return 0;
}

bool CameraBinSession::isMuted() const
{
    return m_muted;
}

void CameraBinSession::setMuted(bool muted)
{
    if (m_muted != muted) {
        m_muted = muted;

        if (m_pipeline)
            g_object_set(G_OBJECT(m_pipeline), MUTE_PROPERTY, m_muted, NULL);
        emit mutedChanged(m_muted);
    }
}

void CameraBinSession::setCaptureDevice(const QString &deviceName)
{
    m_captureDevice = deviceName;
}

void CameraBinSession::setMetaData(const QMap<QByteArray, QVariant> &data)
{
    m_metaData = data;

    if (m_pipeline) {
        GstIterator *elements = gst_bin_iterate_all_by_interface(GST_BIN(m_pipeline), GST_TYPE_TAG_SETTER);
        GstElement *element = 0;
        while (gst_iterator_next(elements, (void**)&element) == GST_ITERATOR_OK) {
            QMapIterator<QByteArray, QVariant> it(data);
            while (it.hasNext()) {
                it.next();
                const QString tagName = it.key();
                const QVariant tagValue = it.value();

                switch(tagValue.type()) {
                    case QVariant::String:
                        gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                            GST_TAG_MERGE_REPLACE_ALL,
                            tagName.toUtf8().constData(),
                            tagValue.toString().toUtf8().constData(),
                            NULL);
                        break;
                    case QVariant::Int:
                    case QVariant::LongLong:
                        gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                            GST_TAG_MERGE_REPLACE_ALL,
                            tagName.toUtf8().constData(),
                            tagValue.toInt(),
                            NULL);
                        break;
                    case QVariant::Double:
                        gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                            GST_TAG_MERGE_REPLACE_ALL,
                            tagName.toUtf8().constData(),
                            tagValue.toDouble(),
                            NULL);
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

bool CameraBinSession::processSyncMessage(const QGstreamerMessage &message)
{
    GstMessage* gm = message.rawMessage();
    const GstStructure *st;
    const GValue *image;
    GstBuffer *buffer = NULL;

    if (gm && GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ELEMENT) {
        if (m_captureMode == QCamera::CaptureStillImage &&
            gst_structure_has_name(gm->structure, "preview-image")) {
            st = gst_message_get_structure(gm);
            if (gst_structure_has_field_typed(st, "buffer", GST_TYPE_BUFFER)) {
                image = gst_structure_get_value(st, "buffer");
                if (image) {
                    buffer = gst_value_get_buffer(image);

                    QImage img;

                    GstCaps *caps = gst_buffer_get_caps(buffer);
                    if (caps) {
                        GstStructure *structure = gst_caps_get_structure(caps, 0);
                        gint width = 0;
                        gint height = 0;

                        if (structure &&
                            gst_structure_get_int(structure, "width", &width) &&
                            gst_structure_get_int(structure, "height", &height) &&
                            width > 0 && height > 0) {
                            if (qstrcmp(gst_structure_get_name(structure), "video/x-raw-rgb") == 0) {
                                QImage::Format format = QImage::Format_Invalid;
                                int bpp = 0;
                                gst_structure_get_int(structure, "bpp", &bpp);

                                if (bpp == 24)
                                    format = QImage::Format_RGB888;
                                else if (bpp == 32)
                                    format = QImage::Format_RGB32;

                                if (format != QImage::Format_Invalid) {
                                    img = QImage((const uchar *)buffer->data, width, height, format);
                                    img.bits(); //detach
                                 }
                            }
                        }
                        gst_caps_unref(caps);

                        static int exposedSignalIndex = metaObject()->indexOfSignal("imageExposed(int)");
                        metaObject()->method(exposedSignalIndex).invoke(this,
                                                                 Qt::QueuedConnection,
                                                                 Q_ARG(int,m_requestId));

                        static int signalIndex = metaObject()->indexOfSignal("imageCaptured(int,QImage)");
                        metaObject()->method(signalIndex).invoke(this,
                                                                 Qt::QueuedConnection,
                                                                 Q_ARG(int,m_requestId),
                                                                 Q_ARG(QImage,img));
                    }

                }
                return true;
            }
        }

        if (gst_structure_has_name(gm->structure, "prepare-xwindow-id")) {
            if (m_viewfinderInterface)
                m_viewfinderInterface->precessNewStream();

            return true;
        }

        if (gst_structure_has_name(gm->structure, GST_PHOTOGRAPHY_AUTOFOCUS_DONE))
            m_cameraFocusControl->handleFocusMessage(gm);

        if (m_viewfinderInterface && GST_MESSAGE_SRC(gm) == GST_OBJECT_CAST(m_viewfinderElement))
            m_viewfinderInterface->handleSyncMessage(gm);
    }

    return false;
}

void CameraBinSession::handleBusMessage(const QGstreamerMessage &message)
{
    GstMessage* gm = message.rawMessage();

    if (gm) {
        if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ERROR) {
            GError *err;
            gchar *debug;
            gst_message_parse_error (gm, &err, &debug);

            QString message;

            if (err && err->message) {
                message = QString::fromUtf8(err->message);
                qWarning() << "CameraBin error:" << message;
            }

            //only report error messager from camerabin
            if (GST_MESSAGE_SRC(gm) == GST_OBJECT_CAST(m_pipeline)) {
                if (message.isEmpty())
                    message = tr("Camera error");

                emit error(int(QMediaRecorder::ResourceError), message);
            }

            if (err)
                g_error_free (err);

            if (debug)
                g_free (debug);
        }

        if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_WARNING) {
            GError *err;
            gchar *debug;
            gst_message_parse_warning (gm, &err, &debug);

            if (err && err->message)
                qWarning() << "CameraBin warning:" << QString::fromUtf8(err->message);

            if (err)
                g_error_free (err);
            if (debug)
                g_free (debug);
        }

        if (GST_MESSAGE_SRC(gm) == GST_OBJECT_CAST(m_pipeline)) {
            switch (GST_MESSAGE_TYPE(gm))  {
            case GST_MESSAGE_DURATION:
                break;

            case GST_MESSAGE_STATE_CHANGED:
                {

                    GstState    oldState;
                    GstState    newState;
                    GstState    pending;

                    gst_message_parse_state_changed(gm, &oldState, &newState, &pending);


#if CAMERABIN_DEBUG
                    QStringList states;
                    states << "GST_STATE_VOID_PENDING" <<  "GST_STATE_NULL" << "GST_STATE_READY" << "GST_STATE_PAUSED" << "GST_STATE_PLAYING";


                    qDebug() << QString("state changed: old: %1  new: %2  pending: %3") \
                            .arg(states[oldState]) \
                           .arg(states[newState]) \
                            .arg(states[pending]);
#endif

                    switch (newState) {
                    case GST_STATE_VOID_PENDING:
                    case GST_STATE_NULL:
                        if (m_state != QCamera::UnloadedState)
                            emit stateChanged(m_state = QCamera::UnloadedState);
                        break;
                    case GST_STATE_READY:
                        if (m_pendingResolutionUpdate) {
                            m_pendingResolutionUpdate = false;
                            setupCaptureResolution();
                            gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
                        }
                        if (m_state != QCamera::LoadedState)
                            emit stateChanged(m_state = QCamera::LoadedState);
                        break;
                    case GST_STATE_PAUSED:
                    case GST_STATE_PLAYING:
                        emit stateChanged(m_state = QCamera::ActiveState);
                        break;
                    }
                }
                break;
            default:
                break;
            }
            //qDebug() << "New session state:" << ENUM_NAME(CameraBinSession,"State",m_state);
        }

        if (m_viewfinderInterface && GST_MESSAGE_SRC(gm) == GST_OBJECT_CAST(m_viewfinderElement))
            m_viewfinderInterface->handleBusMessage(gm);

        emit busMessage(message);
    }
}

void CameraBinSession::recordVideo()
{
    m_recordingActive = true;
    m_actualSink = m_sink;
    if (m_actualSink.isEmpty()) {
        QString ext = m_mediaContainerControl->containerMimeType();
        m_actualSink = generateFileName("clip_", defaultDir(QCamera::CaptureVideo), ext);
    }

    g_object_set(G_OBJECT(m_pipeline), FILENAME_PROPERTY, m_actualSink.toEncoded().constData(), NULL);

    g_signal_emit_by_name(G_OBJECT(m_pipeline), CAPTURE_START, NULL);
}

void CameraBinSession::resumeVideoRecording()
{
    m_recordingActive = true;
    g_signal_emit_by_name(G_OBJECT(m_pipeline), CAPTURE_START, NULL);
}


void CameraBinSession::pauseVideoRecording()
{
    g_signal_emit_by_name(G_OBJECT(m_pipeline), CAPTURE_PAUSE, NULL);
}

void CameraBinSession::stopVideoRecording()
{
    m_recordingActive = false;
    g_signal_emit_by_name(G_OBJECT(m_pipeline), CAPTURE_STOP, NULL);
}

//internal, only used by CameraBinSession::supportedFrameRates.
//recursively fills the list of framerates res from value data.
static void readValue(const GValue *value, QList< QPair<int,int> > *res, bool *continuous)
{
    if (GST_VALUE_HOLDS_FRACTION(value)) {
        int num = gst_value_get_fraction_numerator(value);
        int denum = gst_value_get_fraction_denominator(value);

        *res << QPair<int,int>(num, denum);
    } else if (GST_VALUE_HOLDS_FRACTION_RANGE(value)) {
        const GValue *rateValueMin = gst_value_get_fraction_range_min(value);
        const GValue *rateValueMax = gst_value_get_fraction_range_max(value);

        if (continuous)
            *continuous = true;

        readValue(rateValueMin, res, continuous);
        readValue(rateValueMax, res, continuous);
    } else if (GST_VALUE_HOLDS_LIST(value)) {
        for (uint i=0; i<gst_value_list_get_size(value); i++) {
            readValue(gst_value_list_get_value(value, i), res, continuous);
        }
    }
}

static bool rateLessThan(const QPair<int,int> &r1, const QPair<int,int> &r2)
{
     return r1.first*r2.second < r2.first*r1.second;
}

QList< QPair<int,int> > CameraBinSession::supportedFrameRates(const QSize &frameSize, bool *continuous) const
{
    QList< QPair<int,int> > res;

    if (!m_sourceCaps)
        return res;

    GstCaps *caps = 0;

    if (frameSize.isEmpty()) {
        caps = gst_caps_copy(m_sourceCaps);
    } else {
        GstCaps *filter = gst_caps_new_full(
                gst_structure_new(
                        "video/x-raw-rgb",
                        "width"     , G_TYPE_INT , frameSize.width(),
                        "height"    , G_TYPE_INT, frameSize.height(), NULL),
                gst_structure_new(
                        "video/x-raw-yuv",
                        "width"     , G_TYPE_INT, frameSize.width(),
                        "height"    , G_TYPE_INT, frameSize.height(), NULL),
                gst_structure_new(
                        "image/jpeg",
                        "width"     , G_TYPE_INT, frameSize.width(),
                        "height"    , G_TYPE_INT, frameSize.height(), NULL),
                NULL);

        caps = gst_caps_intersect(m_sourceCaps, filter);
        gst_caps_unref(filter);
    }

    //simplify to the list of rates only:
    gst_caps_make_writable(caps);
    for (uint i=0; i<gst_caps_get_size(caps); i++) {
        GstStructure *structure = gst_caps_get_structure(caps, i);
        gst_structure_set_name(structure, "video/x-raw-yuv");
        const GValue *oldRate = gst_structure_get_value(structure, "framerate");
        GValue rate;
        memset(&rate, 0, sizeof(rate));
        g_value_init(&rate, G_VALUE_TYPE(oldRate));
        g_value_copy(oldRate, &rate);
        gst_structure_remove_all_fields(structure);
        gst_structure_set_value(structure, "framerate", &rate);
    }
    gst_caps_do_simplify(caps);


    for (uint i=0; i<gst_caps_get_size(caps); i++) {
        GstStructure *structure = gst_caps_get_structure(caps, i);
        const GValue *rateValue = gst_structure_get_value(structure, "framerate");
        readValue(rateValue, &res, continuous);
    }

    qSort(res.begin(), res.end(), rateLessThan);

#if CAMERABIN_DEBUG
    qDebug() << "Supported rates:" << gst_caps_to_string(caps);
    qDebug() << res;
#endif

    gst_caps_unref(caps);

    return res;
}

//internal, only used by CameraBinSession::supportedResolutions
//recursively find the supported resolutions range.
static QPair<int,int> valueRange(const GValue *value, bool *continuous)
{
    int minValue = 0;
    int maxValue = 0;

    if (g_value_type_compatible(G_VALUE_TYPE(value), G_TYPE_INT)) {
        minValue = maxValue = g_value_get_int(value);
    } else if (GST_VALUE_HOLDS_INT_RANGE(value)) {
        minValue = gst_value_get_int_range_min(value);
        maxValue = gst_value_get_int_range_max(value);
        *continuous = true;
    } else if (GST_VALUE_HOLDS_LIST(value)) {
        for (uint i=0; i<gst_value_list_get_size(value); i++) {
            QPair<int,int> res = valueRange(gst_value_list_get_value(value, i), continuous);

            if (res.first > 0 && minValue > 0)
                minValue = qMin(minValue, res.first);
            else //select non 0 valid value
                minValue = qMax(minValue, res.first);

            maxValue = qMax(maxValue, res.second);
        }
    }

    return QPair<int,int>(minValue, maxValue);
}

static bool resolutionLessThan(const QSize &r1, const QSize &r2)
{
     return r1.width()*r1.height() < r2.width()*r2.height();
}


QList<QSize> CameraBinSession::supportedResolutions(QPair<int,int> rate,
                                                    bool *continuous,
                                                    QCamera::CaptureMode mode) const
{
    QList<QSize> res;

    if (continuous)
        *continuous = false;

    if (!m_sourceCaps)
        return res;

#if CAMERABIN_DEBUG
    qDebug() << "Source caps:" << gst_caps_to_string(m_sourceCaps);
#endif

    GstCaps *caps = 0;
    bool isContinuous = false;

    if (rate.first <= 0 || rate.second <= 0) {
        caps = gst_caps_copy(m_sourceCaps);
    } else {
        GstCaps *filter = gst_caps_new_full(
                gst_structure_new(
                        "video/x-raw-rgb",
                        "framerate"     , GST_TYPE_FRACTION , rate.first, rate.second, NULL),
                gst_structure_new(
                        "video/x-raw-yuv",
                        "framerate"     , GST_TYPE_FRACTION , rate.first, rate.second, NULL),
                gst_structure_new(
                        "image/jpeg",
                        "framerate"     , GST_TYPE_FRACTION , rate.first, rate.second, NULL),
                NULL);

        caps = gst_caps_intersect(m_sourceCaps, filter);
        gst_caps_unref(filter);
    }

    //simplify to the list of resolutions only:
    gst_caps_make_writable(caps);
    for (uint i=0; i<gst_caps_get_size(caps); i++) {
        GstStructure *structure = gst_caps_get_structure(caps, i);
        gst_structure_set_name(structure, "video/x-raw-yuv");
        const GValue *oldW = gst_structure_get_value(structure, "width");
        const GValue *oldH = gst_structure_get_value(structure, "height");
        GValue w;
        memset(&w, 0, sizeof(GValue));
        GValue h;
        memset(&h, 0, sizeof(GValue));
        g_value_init(&w, G_VALUE_TYPE(oldW));
        g_value_init(&h, G_VALUE_TYPE(oldH));
        g_value_copy(oldW, &w);
        g_value_copy(oldH, &h);
        gst_structure_remove_all_fields(structure);
        gst_structure_set_value(structure, "width", &w);
        gst_structure_set_value(structure, "height", &h);
    }
    gst_caps_do_simplify(caps);

    for (uint i=0; i<gst_caps_get_size(caps); i++) {
        GstStructure *structure = gst_caps_get_structure(caps, i);
        const GValue *wValue = gst_structure_get_value(structure, "width");
        const GValue *hValue = gst_structure_get_value(structure, "height");

        QPair<int,int> wRange = valueRange(wValue, &isContinuous);
        QPair<int,int> hRange = valueRange(hValue, &isContinuous);

        QSize minSize(wRange.first, hRange.first);
        QSize maxSize(wRange.second, hRange.second);

        if (!minSize.isEmpty())
            res << minSize;

        if (minSize != maxSize && !maxSize.isEmpty())
            res << maxSize;
    }


    qSort(res.begin(), res.end(), resolutionLessThan);

    //if the range is continuos, populate is with the common rates
    if (isContinuous && res.size() >= 2) {
        //fill the ragne with common value
        static QList<QSize> commonSizes =
                QList<QSize>() << QSize(128, 96)
                               << QSize(160,120)
                               << QSize(176, 144)
                               << QSize(320, 240)
                               << QSize(352, 288)
                               << QSize(640, 480)
                               << QSize(848, 480)
                               << QSize(854, 480)
                               << QSize(1024, 768)
                               << QSize(1280, 720) // HD 720
                               << QSize(1280, 1024)
                               << QSize(1600, 1200)
                               << QSize(1920, 1080) // HD
                               << QSize(1920, 1200)
                               << QSize(2048, 1536)
                               << QSize(2560, 1600)
                               << QSize(2580, 1936);
        QSize minSize = res.first();
        QSize maxSize = res.last();

#ifdef Q_WS_MAEMO_5
        if (mode == QCamera::CaptureVideo && cameraRole() == BackCamera)
            maxSize = QSize(848, 480);
        if (mode == QCamera::CaptureStillImage)
            minSize = QSize(640, 480);
#elif defined(Q_WS_MAEMO_6)
        if (cameraRole() == FrontCamera && maxSize.width() > 640)
            maxSize = QSize(640, 480);
        else if (mode == QCamera::CaptureVideo && maxSize.width() > 1280)
            maxSize = QSize(1280, 720);
#else
        Q_UNUSED(mode);
#endif

        res.clear();

        foreach (const QSize &candidate, commonSizes) {
            int w = candidate.width();
            int h = candidate.height();

            if (w > maxSize.width() && h > maxSize.height())
                break;

            if (w >= minSize.width() && h >= minSize.height() &&
                w <= maxSize.width() && h <= maxSize.height())
                res << candidate;
        }

        if (res.isEmpty() || res.first() != minSize)
            res.prepend(minSize);

        if (res.last() != maxSize)
            res.append(maxSize);
    }

#if CAMERABIN_DEBUG
    qDebug() << "Supported resolutions:" << gst_caps_to_string(caps);
    qDebug() << res;
#endif

    gst_caps_unref(caps);

    if (continuous)
        *continuous = isContinuous;

    return res;
}

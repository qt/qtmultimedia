/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
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

#include "qmediarecorder.h"
#include "qmediarecorder_p.h"

#include <qmediarecordercontrol.h>
#include "qmediaobject_p.h"
#include <qmediaservice.h>
#include <qmediaserviceprovider.h>
#include <qmetadatawritercontrol.h>
#include <qaudioencodercontrol.h>
#include <qvideoencodercontrol.h>
#include <qmediacontainercontrol.h>
#include <qcamera.h>
#include <qcameracontrol.h>

#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qmetaobject.h>

#include <qaudioformat.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMediaRecorder
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_recording

    \brief The QMediaRecorder class is used for the recording of media content.

    The QMediaRecorder class is a high level media recording class.  It's not
    intended to be used alone but for accessing the media recording functions
    of other media objects, like QRadioTuner, or QCamera.

    \snippet doc/src/snippets/multimedia-snippets/media.cpp Media recorder

    \sa QAudioRecorder
*/

namespace
{
class MediaRecorderRegisterMetaTypes
{
public:
    MediaRecorderRegisterMetaTypes()
    {
        qRegisterMetaType<QMediaRecorder::State>("QMediaRecorder::State");
        qRegisterMetaType<QMediaRecorder::Error>("QMediaRecorder::Error");
    }
} _registerRecorderMetaTypes;
}

QMediaRecorderPrivate::QMediaRecorderPrivate():
     mediaObject(0),
     control(0),
     formatControl(0),
     audioControl(0),
     videoControl(0),
     metaDataControl(0),
     notifyTimer(0),
     state(QMediaRecorder::StoppedState),
     error(QMediaRecorder::NoError)
{
}

#define ENUM_NAME(c,e,v) (c::staticMetaObject.enumerator(c::staticMetaObject.indexOfEnumerator(e)).valueToKey((v)))

void QMediaRecorderPrivate::_q_stateChanged(QMediaRecorder::State ps)
{
    Q_Q(QMediaRecorder);

    if (ps == QMediaRecorder::RecordingState)
        notifyTimer->start();
    else
        notifyTimer->stop();

//    qDebug() << "Recorder state changed:" << ENUM_NAME(QMediaRecorder,"State",ps);
    if (state != ps) {
        emit q->stateChanged(ps);
    }

    state = ps;
}


void QMediaRecorderPrivate::_q_error(int error, const QString &errorString)
{
    Q_Q(QMediaRecorder);

    this->error = QMediaRecorder::Error(error);
    this->errorString = errorString;

    emit q->error(this->error);
}

void QMediaRecorderPrivate::_q_serviceDestroyed()
{
    mediaObject = 0;
    control = 0;
    formatControl = 0;
    audioControl = 0;
    videoControl = 0;
    metaDataControl = 0;
}

void QMediaRecorderPrivate::_q_notify()
{
    emit q_func()->durationChanged(q_func()->duration());
}

void QMediaRecorderPrivate::_q_updateNotifyInterval(int ms)
{
    notifyTimer->setInterval(ms);
}


/*!
    Constructs a media recorder which records the media produced by \a mediaObject.

    The \a parent is passed to QMediaObject.
*/

QMediaRecorder::QMediaRecorder(QMediaObject *mediaObject, QObject *parent):
    QObject(parent),
    d_ptr(new QMediaRecorderPrivate)
{
    Q_D(QMediaRecorder);
    d->q_ptr = this;

    d->notifyTimer = new QTimer(this);
    connect(d->notifyTimer, SIGNAL(timeout()), SLOT(_q_notify()));

    setMediaObject(mediaObject);
}

/*!
    \internal
*/
QMediaRecorder::QMediaRecorder(QMediaRecorderPrivate &dd, QMediaObject *mediaObject, QObject *parent):
    QObject(parent),
    d_ptr(&dd)
{
    Q_D(QMediaRecorder);
    d->q_ptr = this;

    d->notifyTimer = new QTimer(this);
    connect(d->notifyTimer, SIGNAL(timeout()), SLOT(_q_notify()));

    setMediaObject(mediaObject);
}

/*!
    Destroys a media recorder object.
*/

QMediaRecorder::~QMediaRecorder()
{
}

/*!
    Returns the QMediaObject instance that this QMediaRecorder is bound too,
    or 0 otherwise.
*/
QMediaObject *QMediaRecorder::mediaObject() const
{
    return d_func()->mediaObject;
}

/*!
    \internal
*/
bool QMediaRecorder::setMediaObject(QMediaObject *object)
{
    Q_D(QMediaRecorder);

    if (object == d->mediaObject)
        return true;

    if (d->mediaObject) {
        if (d->control) {
            disconnect(d->control, SIGNAL(stateChanged(QMediaRecorder::State)),
                       this, SLOT(_q_stateChanged(QMediaRecorder::State)));

            disconnect(d->control, SIGNAL(mutedChanged(bool)),
                       this, SIGNAL(mutedChanged(bool)));

            disconnect(d->control, SIGNAL(durationChanged(qint64)),
                       this, SIGNAL(durationChanged(qint64)));

            disconnect(d->control, SIGNAL(error(int,QString)),
                       this, SLOT(_q_error(int,QString)));
        }

        disconnect(d->mediaObject, SIGNAL(notifyIntervalChanged(int)), this, SLOT(_q_updateNotifyInterval(int)));

        QMediaService *service = d->mediaObject->service();

        if (service) {
            disconnect(service, SIGNAL(destroyed()), this, SLOT(_q_serviceDestroyed()));

            if (d->control)
                service->releaseControl(d->control);
            if (d->formatControl)
                service->releaseControl(d->formatControl);
            if (d->audioControl)
                service->releaseControl(d->audioControl);
            if (d->videoControl)
                service->releaseControl(d->videoControl);
            if (d->metaDataControl) {
                disconnect(d->metaDataControl, SIGNAL(metaDataChanged()),
                        this, SIGNAL(metaDataChanged()));
                disconnect(d->metaDataControl, SIGNAL(metaDataAvailableChanged(bool)),
                        this, SIGNAL(metaDataAvailableChanged(bool)));
                disconnect(d->metaDataControl, SIGNAL(writableChanged(bool)),
                        this, SIGNAL(metaDataWritableChanged(bool)));

                service->releaseControl(d->metaDataControl);
            }
        }
    }

    d->control = 0;
    d->formatControl = 0;
    d->audioControl = 0;
    d->videoControl = 0;
    d->metaDataControl = 0;

    d->mediaObject = object;

    if (d->mediaObject) {
        QMediaService *service = d->mediaObject->service();

        d->notifyTimer->setInterval(d->mediaObject->notifyInterval());
        connect(d->mediaObject, SIGNAL(notifyIntervalChanged(int)), SLOT(_q_updateNotifyInterval(int)));

        if (service) {
            d->control = qobject_cast<QMediaRecorderControl*>(service->requestControl(QMediaRecorderControl_iid));

            if (d->control) {
                d->formatControl = qobject_cast<QMediaContainerControl *>(service->requestControl(QMediaContainerControl_iid));
                d->audioControl = qobject_cast<QAudioEncoderControl *>(service->requestControl(QAudioEncoderControl_iid));
                d->videoControl = qobject_cast<QVideoEncoderControl *>(service->requestControl(QVideoEncoderControl_iid));

                QMediaControl *control = service->requestControl(QMetaDataWriterControl_iid);
                if (control) {
                    d->metaDataControl = qobject_cast<QMetaDataWriterControl *>(control);
                    if (!d->metaDataControl) {
                        service->releaseControl(control);
                    } else {
                        connect(d->metaDataControl,
                                SIGNAL(metaDataChanged()),
                                SIGNAL(metaDataChanged()));
                        connect(d->metaDataControl,
                                SIGNAL(metaDataAvailableChanged(bool)),
                                SIGNAL(metaDataAvailableChanged(bool)));
                        connect(d->metaDataControl,
                                SIGNAL(writableChanged(bool)),
                                SIGNAL(metaDataWritableChanged(bool)));
                    }
                }

                connect(d->control, SIGNAL(stateChanged(QMediaRecorder::State)),
                        this, SLOT(_q_stateChanged(QMediaRecorder::State)));

                connect(d->control, SIGNAL(mutedChanged(bool)),
                        this, SIGNAL(mutedChanged(bool)));

                connect(d->control, SIGNAL(durationChanged(qint64)),
                        this, SIGNAL(durationChanged(qint64)));

                connect(d->control, SIGNAL(error(int,QString)),
                        this, SLOT(_q_error(int,QString)));

                connect(service, SIGNAL(destroyed()), this, SLOT(_q_serviceDestroyed()));


                return true;
            }
        }

        d->mediaObject = 0;
        return false;
    }

    return true;
}

/*!
    \property QMediaRecorder::outputLocation
    \brief the destination location of media content.

    Setting the location can fail, for example when the service supports only
    local file system locations but a network URL was passed. If the service
    does not support media recording this setting the output location will
    always fail.

    The \a location can be relative or empty;
    in this case the recorder uses the system specific place and file naming scheme.
    After recording has stated, QMediaRecorder::outputLocation() returns the actual output location.
*/

/*!
    Returns true if media recorder service ready to use.
*/
bool QMediaRecorder::isAvailable() const
{
    if (d_func()->control != NULL)
        return true;
    else
        return false;
}

/*!
    Returns the availability error code.
*/
QtMultimedia::AvailabilityError QMediaRecorder::availabilityError() const
{
    if (d_func()->control != NULL)
        return QtMultimedia::NoError;
    else
        return QtMultimedia::ServiceMissingError;
}

QUrl QMediaRecorder::outputLocation() const
{
    return d_func()->control ? d_func()->control->outputLocation() : QUrl();
}

bool QMediaRecorder::setOutputLocation(const QUrl &location)
{
    Q_D(QMediaRecorder);
    return d->control ? d->control->setOutputLocation(location) : false;
}

/*!
    Returns the current media recorder state.

    \sa QMediaRecorder::State
*/

QMediaRecorder::State QMediaRecorder::state() const
{
    return d_func()->control ? QMediaRecorder::State(d_func()->control->state()) : StoppedState;
}

/*!
    Returns the current error state.

    \sa errorString()
*/

QMediaRecorder::Error QMediaRecorder::error() const
{
    return d_func()->error;
}

/*!
    Returns a string describing the current error state.

    \sa error()
*/

QString QMediaRecorder::errorString() const
{
    return d_func()->errorString;
}

/*!
    \property QMediaRecorder::duration

    \brief the recorded media duration in milliseconds.
*/

qint64 QMediaRecorder::duration() const
{
    return d_func()->control ? d_func()->control->duration() : 0;
}

/*!
    \property QMediaRecorder::muted

    \brief whether a recording audio stream is muted.
*/

bool QMediaRecorder::isMuted() const
{
    return d_func()->control ? d_func()->control->isMuted() : 0;
}

void QMediaRecorder::setMuted(bool muted)
{
    Q_D(QMediaRecorder);

    if (d->control)
        d->control->setMuted(muted);
}

/*!
    Returns a list of MIME types of supported container formats.
*/
QStringList QMediaRecorder::supportedContainers() const
{
    return d_func()->formatControl ?
           d_func()->formatControl->supportedContainers() : QStringList();
}

/*!
    Returns a description of a container format \a mimeType.
*/
QString QMediaRecorder::containerDescription(const QString &mimeType) const
{
    return d_func()->formatControl ?
           d_func()->formatControl->containerDescription(mimeType) : QString();
}

/*!
    Returns the MIME type of the selected container format.
*/

QString QMediaRecorder::containerMimeType() const
{
    return d_func()->formatControl ?
           d_func()->formatControl->containerMimeType() : QString();
}

/*!
    Returns a list of supported audio codecs.
*/
QStringList QMediaRecorder::supportedAudioCodecs() const
{
    return d_func()->audioControl ?
           d_func()->audioControl->supportedAudioCodecs() : QStringList();
}

/*!
    Returns a description of an audio \a codec.
*/
QString QMediaRecorder::audioCodecDescription(const QString &codec) const
{
    return d_func()->audioControl ?
           d_func()->audioControl->codecDescription(codec) : QString();
}

/*!
    Returns a list of supported audio sample rates.

    If non null audio \a settings parameter is passed, the returned list is
    reduced to sample rates supported with partial settings applied.

    This can be used to query the list of sample rates, supported by specific
    audio codec.

    If the encoder supports arbitrary sample rates within the supported rates
    range, *\a continuous is set to true, otherwise *\a continuous is set to
    false.
*/

QList<int> QMediaRecorder::supportedAudioSampleRates(const QAudioEncoderSettings &settings, bool *continuous) const
{
    if (continuous)
        *continuous = false;

    return d_func()->audioControl ?
           d_func()->audioControl->supportedSampleRates(settings, continuous) : QList<int>();
}

/*!
    Returns a list of resolutions video can be encoded at.

    If non null video \a settings parameter is passed, the returned list is
    reduced to resolution supported with partial settings like video codec or
    framerate applied.

    If the encoder supports arbitrary resolutions within the supported range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.

    \sa QVideoEncoderSettings::resolution()
*/
QList<QSize> QMediaRecorder::supportedResolutions(const QVideoEncoderSettings &settings, bool *continuous) const
{
    if (continuous)
        *continuous = false;

    return d_func()->videoControl ?
           d_func()->videoControl->supportedResolutions(settings, continuous) : QList<QSize>();
}

/*!
    Returns a list of frame rates video can be encoded at.

    If non null video \a settings parameter is passed, the returned list is
    reduced to frame rates supported with partial settings like video codec or
    resolution applied.

    If the encoder supports arbitrary frame rates within the supported range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.

    \sa QVideoEncoderSettings::frameRate()
*/
QList<qreal> QMediaRecorder::supportedFrameRates(const QVideoEncoderSettings &settings, bool *continuous) const
{
    if (continuous)
        *continuous = false;

    return d_func()->videoControl ?
           d_func()->videoControl->supportedFrameRates(settings, continuous) : QList<qreal>();
}

/*!
    Returns a list of supported video codecs.
*/
QStringList QMediaRecorder::supportedVideoCodecs() const
{
    return d_func()->videoControl ?
           d_func()->videoControl->supportedVideoCodecs() : QStringList();
}

/*!
    Returns a description of a video \a codec.

    \sa setEncodingSettings()
*/
QString QMediaRecorder::videoCodecDescription(const QString &codec) const
{
    return d_func()->videoControl ?
           d_func()->videoControl->videoCodecDescription(codec) : QString();
}

/*!
    Returns the audio encoder settings being used.

    \sa setEncodingSettings()
*/

QAudioEncoderSettings QMediaRecorder::audioSettings() const
{
    return d_func()->audioControl ?
           d_func()->audioControl->audioSettings() : QAudioEncoderSettings();
}

/*!
    Returns the video encoder settings being used.

    \sa setEncodingSettings()
*/

QVideoEncoderSettings QMediaRecorder::videoSettings() const
{
    return d_func()->videoControl ?
           d_func()->videoControl->videoSettings() : QVideoEncoderSettings();
}

/*!
    Sets the \a audio and \a video encoder settings and \a container format MIME type.

    If some parameters are not specified, or null settings are passed, the
    encoder will choose default encoding parameters, depending on media
    source properties.
    While setEncodingSettings is optional, the backend can preload
    encoding pipeline to improve recording startup time.

    It's only possible to change settings when the encoder is in the
    QMediaEncoder::StoppedState state.

    \sa audioSettings(), videoSettings(), containerMimeType()
*/

void QMediaRecorder::setEncodingSettings(const QAudioEncoderSettings &audio,
                                         const QVideoEncoderSettings &video,
                                         const QString &container)
{
    Q_D(QMediaRecorder);

    QCamera *camera = qobject_cast<QCamera*>(d->mediaObject);
    if (camera && camera->captureMode() == QCamera::CaptureVideo) {
        QMetaObject::invokeMethod(camera,
                                  "_q_preparePropertyChange",
                                  Qt::DirectConnection,
                                  Q_ARG(int, QCameraControl::VideoEncodingSettings));
    }

    if (d->audioControl)
        d->audioControl->setAudioSettings(audio);

    if (d->videoControl)
        d->videoControl->setVideoSettings(video);

    if (d->formatControl)
        d->formatControl->setContainerMimeType(container);

    if (d->control)
        d->control->applySettings();
}


/*!
    Start recording.

    This is an asynchronous call, with signal
    stateCahnged(QMediaRecorder::RecordingState) being emitted when recording
    started, otherwise the error() signal is emitted.
*/

void QMediaRecorder::record()
{
    Q_D(QMediaRecorder);

    // reset error
    d->error = NoError;
    d->errorString = QString();

    if (d->control)
        d->control->record();
}

/*!
    Pause recording.
*/

void QMediaRecorder::pause()
{
    Q_D(QMediaRecorder);
    if (d->control)
        d->control->pause();
}

/*!
    Stop recording.
*/

void QMediaRecorder::stop()
{
    Q_D(QMediaRecorder);
    if (d->control)
        d->control->stop();
}

/*!
    \enum QMediaRecorder::State

    \value StoppedState    The recorder is not active.
    \value RecordingState  The recorder is currently active and producing data.
    \value PausedState     The recorder is paused.
*/

/*!
    \enum QMediaRecorder::Error

    \value NoError         No Errors.
    \value ResourceError   Device is not ready or not available.
    \value FormatError     Current format is not supported.
*/

/*!
    \fn QMediaRecorder::stateChanged(State state)

    Signals that a media recorder's \a state has changed.
*/

/*!
    \fn QMediaRecorder::durationChanged(qint64 duration)

    Signals that the \a duration of the recorded media has changed.
*/

/*!
    \fn QMediaRecorder::error(QMediaRecorder::Error error)

    Signals that an \a error has occurred.
*/

/*!
    \fn QMediaRecorder::mutedChanged(bool muted)

    Signals that the \a muted state has changed. If true the recording is being muted.
*/

/*!
    \property QMediaRecorder::metaDataAvailable
    \brief whether access to a media object's meta-data is available.

    If this is true there is meta-data available, otherwise there is no meta-data available.
*/

bool QMediaRecorder::isMetaDataAvailable() const
{
    Q_D(const QMediaRecorder);

    return d->metaDataControl
            ? d->metaDataControl->isMetaDataAvailable()
            : false;
}

/*!
    \fn QMediaRecorder::metaDataAvailableChanged(bool available)

    Signals that the \a available state of a media object's meta-data has changed.
*/

/*!
    \property QMediaRecorder::metaDataWritable
    \brief whether a media object's meta-data is writable.

    If this is true the meta-data is writable, otherwise the meta-data is read-only.
*/

bool QMediaRecorder::isMetaDataWritable() const
{
    Q_D(const QMediaRecorder);

    return d->metaDataControl
            ? d->metaDataControl->isWritable()
            : false;
}

/*!
    \fn QMediaRecorder::metaDataWritableChanged(bool writable)

    Signals that the \a writable state of a media object's meta-data has changed.
*/

/*!
    Returns the value associated with a meta-data \a key.
*/
QVariant QMediaRecorder::metaData(QtMultimedia::MetaData key) const
{
    Q_D(const QMediaRecorder);

    return d->metaDataControl
            ? d->metaDataControl->metaData(key)
            : QVariant();
}

/*!
    Sets a \a value for a meta-data \a key.

    \note To ensure that meta data is set corretly, it should be set before starting the recording.
    Once the recording is stopped, any meta data set will be attached to the next recording.
*/
void QMediaRecorder::setMetaData(QtMultimedia::MetaData key, const QVariant &value)
{
    Q_D(QMediaRecorder);

    if (d->metaDataControl)
        d->metaDataControl->setMetaData(key, value);
}

/*!
    Returns a list of keys there is meta-data available for.
*/
QList<QtMultimedia::MetaData> QMediaRecorder::availableMetaData() const
{
    Q_D(const QMediaRecorder);

    return d->metaDataControl
            ? d->metaDataControl->availableMetaData()
            : QList<QtMultimedia::MetaData>();
}

/*!
    \fn QMediaRecorder::metaDataChanged()

    Signals that a media object's meta-data has changed.
*/

/*!
    Returns the value associated with a meta-data \a key.

    The naming and type of extended meta-data is not standardized, so the values and meaning
    of keys may vary between backends.
*/
QVariant QMediaRecorder::extendedMetaData(const QString &key) const
{
    Q_D(const QMediaRecorder);

    return d->metaDataControl
            ? d->metaDataControl->extendedMetaData(key)
            : QVariant();
}

/*!
    Sets a \a value for a meta-data \a key.

    The naming and type of extended meta-data is not standardized, so the values and meaning
    of keys may vary between backends.
*/
void QMediaRecorder::setExtendedMetaData(const QString &key, const QVariant &value)
{
    Q_D(QMediaRecorder);

    if (d->metaDataControl)
        d->metaDataControl->setExtendedMetaData(key, value);
}

/*!
    Returns a list of keys there is extended meta-data available for.
*/
QStringList QMediaRecorder::availableExtendedMetaData() const
{
    Q_D(const QMediaRecorder);

    return d->metaDataControl
            ? d->metaDataControl->availableExtendedMetaData()
            : QStringList();
}

#include "moc_qmediarecorder.cpp"
QT_END_NAMESPACE


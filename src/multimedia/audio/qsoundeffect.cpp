// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsoundeffect.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qfuture.h>
#include <QtMultimedia/qaudiobuffer.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qsamplecache_p.h>
#include <QtMultimedia/private/qsoundeffectsynchronous_p.h>
#include <QtMultimedia/private/qsoundeffectwithplayer_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

QT_BEGIN_NAMESPACE

Q_APPLICATION_STATIC(QSampleCache, sampleCache)
Q_LOGGING_CATEGORY(qLcSoundEffect, "qt.multimedia.soundeffect")

namespace {

QSoundEffectPrivate *makeSoundEffectPrivate(QSoundEffect *fx, const QAudioDevice &audioDevice)
{
#if defined(Q_OS_MACOS) && defined(Q_OS_WIN)
    return new QtMultimediaPrivate::QSoundEffectPrivateWithPlayer(fx, audioDevice);
#endif

    QAudioSink dummySink(audioDevice.isNull() ? QMediaDevices::defaultAudioOutput() : audioDevice);
    auto platformSink = QPlatformAudioSink::get(dummySink);

    if (platformSink && platformSink->hasCallbackAPI())
        return new QtMultimediaPrivate::QSoundEffectPrivateWithPlayer(fx, audioDevice);
    else
        return new QSoundEffectPrivateSynchronous(fx, audioDevice);
}

} // namespace

/*!
    \class QSoundEffect
    \brief The QSoundEffect class provides a way to play low latency sound effects.

    \ingroup multimedia
    \ingroup multimedia_audio
    \inmodule QtMultimedia

    This class allows you to play uncompressed audio files (typically WAV files) in
    a generally lower latency way, and is suitable for "feedback" type sounds in
    response to user actions (e.g. virtual keyboard sounds, positive or negative
    feedback for popup dialogs, or game sounds).  If low latency is not important,
    consider using the QMediaPlayer class instead, since it supports a wider
    variety of media formats and is less resource intensive.

    This example shows how a looping, somewhat quiet sound effect
    can be played:

    \snippet multimedia-snippets/qsound.cpp 2

    Typically the sound effect should be reused, which allows all the
    parsing and preparation to be done ahead of time, and only triggered
    when necessary.  This assists with lower latency audio playback.

    \snippet multimedia-snippets/qsound.cpp 3

    Since QSoundEffect requires slightly more resources to achieve lower
    latency playback, the platform may limit the number of simultaneously playing
    sound effects.
*/


/*!
    \qmltype SoundEffect
    \nativetype QSoundEffect
    \brief The SoundEffect type provides a way to play sound effects in QML.

    \inmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_audio_qml
    \inqmlmodule QtMultimedia

    This type allows you to play uncompressed audio files (typically WAV files) in
    a generally lower latency way, and is suitable for "feedback" type sounds in
    response to user actions (e.g. virtual keyboard sounds, positive or negative
    feedback for popup dialogs, or game sounds).  If low latency is not important,
    consider using the MediaPlayer type instead, since it support a wider
    variety of media formats and is less resource intensive.

    Typically the sound effect should be reused, which allows all the
    parsing and preparation to be done ahead of time, and only triggered
    when necessary.  This is easy to achieve with QML, since you can declare your
    SoundEffect instance and refer to it elsewhere.

    The following example plays a WAV file on mouse click.

    \snippet multimedia-snippets/soundeffect.qml complete snippet

    Since SoundEffect requires slightly more resources to achieve lower
    latency playback, the platform may limit the number of simultaneously playing
    sound effects.
*/

/*!
    Creates a QSoundEffect with the given \a parent.
*/
QSoundEffect::QSoundEffect(QObject *parent)
    : QSoundEffect(QAudioDevice(), parent)
{
}

/*!
    Creates a QSoundEffect with the given \a audioDevice and \a parent.
*/
QSoundEffect::QSoundEffect(const QAudioDevice &audioDevice, QObject *parent)
    : QObject(*makeSoundEffectPrivate(this, audioDevice), parent)
{
}

/*!
    Destroys this sound effect.
 */
QSoundEffect::~QSoundEffect()
{
    stop();
}

/*!
    \fn QSoundEffect::supportedMimeTypes()

    Returns a list of the supported mime types for this platform.
*/
QStringList QSoundEffect::supportedMimeTypes()
{
    // Only return supported mime types if we have a audio device available
    const QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    if (devices.isEmpty())
        return QStringList();

    using namespace Qt::Literals;
    static const QStringList mimeTypes{
        u"audio/aiff"_s,
        u"audio/vnd.wave"_s,
        u"audio/wav"_s,
        u"audio/wave"_s,
        u"audio/x-aiff"_s
        u"audio/x-pn-wav"_s,
        u"audio/x-wav"_s,
    };

    return mimeTypes;
}

/*!
    \qmlproperty url QtMultimedia::SoundEffect::source

    This property holds the url for the sound to play. For the SoundEffect
    to attempt to load the source, the URL must exist and the application must have read permission
    in the specified directory. If the desired source is a local file the URL may be specified
    using either absolute or relative (to the file that declared the SoundEffect) pathing.
*/
/*!
    \property QSoundEffect::source

    This property holds the url for the sound to play. For the SoundEffect
    to attempt to load the source, the URL must exist and the application must have read permission
    in the specified directory.
*/

/*! Returns the URL of the current source to play */
QUrl QSoundEffect::source() const
{
    Q_D(const QSoundEffect);

    return d->url();
}

/*! Set the current URL to play to \a url. */
void QSoundEffect::setSource(const QUrl &url)
{
    Q_D(QSoundEffect);

    qCDebug(qLcSoundEffect) << this << "setSource current=" << d->url() << ", to=" << url;
    if (d->url() == url)
        return;

    stop();

    if (d->setSource(url, *sampleCache()))
        emit sourceChanged();
}

/*!
    \qmlproperty int QtMultimedia::SoundEffect::loops

    This property holds the number of times the sound is played. A value of 0 or 1 means
    the sound will be played only once; set to SoundEffect.Infinite to enable infinite looping.

    The value can be changed while the sound effect is playing, in which case it will update
    the remaining loops to the new value.
*/

/*!
    \property QSoundEffect::loops
    This property holds the number of times the sound is played. A value of 0 or 1 means
    the sound will be played only once; set to SoundEffect.Infinite to enable infinite looping.

    The value can be changed while the sound effect is playing, in which case it will update
    the remaining loops to the new value.
*/

/*!
    Returns the total number of times that this sound effect will be played before stopping.

    See the \l loopsRemaining() method for the number of loops currently remaining.
 */
int QSoundEffect::loopCount() const
{
    Q_D(const QSoundEffect);
    return d->loopCount();
}

/*!
    \enum QSoundEffect::Loop

    \value Infinite  Used as a parameter to \l setLoopCount() for infinite looping
*/

/*!
    Set the total number of times to play this sound effect to \a loopCount.

    Setting the loop count to 0 or 1 means the sound effect will be played only once;
    pass \c QSoundEffect::Infinite to repeat indefinitely. The loop count can be changed while
    the sound effect is playing, in which case it will update the remaining loops to
    the new \a loopCount.

    \sa loopsRemaining()
*/
void QSoundEffect::setLoopCount(int loopCount)
{
    Q_D(QSoundEffect);

    if (loopCount < 0 && loopCount != Infinite) {
        qWarning("SoundEffect: loops should be SoundEffect.Infinite, 0 or positive integer");
        return;
    }

    if (d->setLoopCount(loopCount))
        emit loopCountChanged();
}

/*!
    \property QSoundEffect::audioDevice

    Returns the QAudioDevice instance.
*/
QAudioDevice QSoundEffect::audioDevice()
{
    Q_D(const QSoundEffect);
    return d->audioDevice();
}

void QSoundEffect::setAudioDevice(const QAudioDevice &device)
{
    Q_D(QSoundEffect);

    qCDebug(qLcSoundEffect) << this << "setAudioDevice:" << device.description();

    if (d->setAudioDevice(device))
        emit audioDeviceChanged();
}

/*!
    \qmlproperty int QtMultimedia::SoundEffect::loopsRemaining

    This property contains the number of loops remaining before the sound effect
    stops by itself, or SoundEffect.Infinite if that's what has been set in \l loops.
*/
/*!
    \property QSoundEffect::loopsRemaining

    This property contains the number of loops remaining before the sound effect
    stops by itself, or QSoundEffect::Infinite if that's what has been set in \l loops.
*/
int QSoundEffect::loopsRemaining() const
{
    Q_D(const QSoundEffect);
    return d->loopsRemaining();
}

/*!
    \qmlproperty real QtMultimedia::SoundEffect::volume

    This property holds the volume of the sound effect playback.

    The volume is scaled linearly from \c 0.0 (silence) to \c 1.0 (full volume). Values outside this
    range will be clamped.

    The default volume is \c 1.0.

    UI volume controls should usually be scaled non-linearly. For example, using a logarithmic scale
    will produce linear changes in perceived loudness, which is what a user would normally expect
    from a volume control. See \l {QtAudio::convertVolume()}{convertVolume()}
    for more details.
*/
/*!
    \property QSoundEffect::volume

    This property holds the volume of the sound effect playback, from 0.0 (silence) to 1.0 (full volume).
*/

/*!
    Returns the current volume of this sound effect, from 0.0 (silent) to 1.0 (maximum volume).
 */
float QSoundEffect::volume() const
{
    Q_D(const QSoundEffect);
    return d->volume();
}

/*!
    Sets the sound effect volume to \a volume.

    The volume is scaled linearly from \c 0.0 (silence) to \c 1.0 (full volume). Values outside this
    range will be clamped.

    The default volume is \c 1.0.

    UI volume controls should usually be scaled non-linearly. For example, using a logarithmic scale
    will produce linear changes in perceived loudness, which is what a user would normally expect
    from a volume control. See QtAudio::convertVolume() for more details.
 */
void QSoundEffect::setVolume(float volume)
{
    Q_D(QSoundEffect);
    if (d->setVolume(volume))
        emit volumeChanged();
}

/*!
    \qmlproperty bool QtMultimedia::SoundEffect::muted

    This property provides a way to control muting. A value of \c true will mute this effect.
    Otherwise, playback will occur with the currently specified \l volume.
*/
/*!
    \property QSoundEffect::muted

    This property provides a way to control muting. A value of \c true will mute this effect.
*/

/*! Returns whether this sound effect is muted */
bool QSoundEffect::isMuted() const
{
    Q_D(const QSoundEffect);
    return d->muted();
}

/*!
    Sets whether to mute this sound effect's playback.

    If \a muted is true, playback will be muted (silenced),
    and otherwise playback will occur with the currently
    specified volume().
*/
void QSoundEffect::setMuted(bool muted)
{
    Q_D(QSoundEffect);
    if (d->setMuted(muted))
        emit mutedChanged();
}

/*!
    \fn QSoundEffect::isLoaded() const

    Returns whether the sound effect has finished loading the \l source().
*/
/*!
    \qmlmethod bool QtMultimedia::SoundEffect::isLoaded()

    Returns whether the sound effect has finished loading the \l source.
*/
bool QSoundEffect::isLoaded() const
{
    Q_D(const QSoundEffect);
    return d->status() == QSoundEffect::Ready;
}

/*!
    \qmlmethod QtMultimedia::SoundEffect::play()

    Start playback of the sound effect, looping the effect for the number of
    times as specified in the loops property.

    This is the default method for SoundEffect.

    \snippet multimedia-snippets/soundeffect.qml play sound on click
*/
/*!
    \fn QSoundEffect::play()

    Start playback of the sound effect, looping the effect for the number of
    times as specified in the loops property.
*/
void QSoundEffect::play()
{
    Q_D(QSoundEffect);
    d->play();
}

/*!
    \qmlproperty bool QtMultimedia::SoundEffect::playing

    This property indicates whether the sound effect is playing or not.
*/
/*!
    \property QSoundEffect::playing

    This property indicates whether the sound effect is playing or not.
*/

/*! Returns true if the sound effect is currently playing, or false otherwise */
bool QSoundEffect::isPlaying() const
{
    Q_D(const QSoundEffect);
    return d->playing();
}

/*!
    \enum QSoundEffect::Status

    \value Null  No source has been set or the source is null.
    \value Loading  The SoundEffect is trying to load the source.
    \value Ready  The source is loaded and ready for play.
    \value Error  An error occurred during operation, such as failure of loading the source.

*/

/*!
    \qmlproperty enumeration QtMultimedia::SoundEffect::status

    This property indicates the current status of the SoundEffect
    as enumerated within SoundEffect.
    Possible statuses are listed below.

    \table
    \header \li Value \li Description
    \row \li SoundEffect.Null    \li No source has been set or the source is null.
    \row \li SoundEffect.Loading \li The SoundEffect is trying to load the source.
    \row \li SoundEffect.Ready   \li The source is loaded and ready for play.
    \row \li SoundEffect.Error   \li An error occurred during operation, such as failure of loading the source.
    \endtable
*/
/*!
    \property QSoundEffect::status

    This property indicates the current status of the sound effect
    from the \l QSoundEffect::Status enumeration.
*/

/*!
    Returns the current status of this sound effect.
 */
QSoundEffect::Status QSoundEffect::status() const
{
    Q_D(const QSoundEffect);
    return d->status();
}

/*!
  \qmlmethod QtMultimedia::SoundEffect::stop()

  Stop current playback.

 */
/*!
  \fn QSoundEffect::stop()

  Stop current playback.

 */
void QSoundEffect::stop()
{
    Q_D(QSoundEffect);
    d->stop();
}

/* Signals */

/*!
    \fn void QSoundEffect::sourceChanged()

    The \c sourceChanged signal is emitted when the source has been changed.
*/
/*!
    \qmlsignal QtMultimedia::SoundEffect::sourceChanged()

    The \c sourceChanged signal is emitted when the source has been changed.
*/
/*!
    \fn void QSoundEffect::loadedChanged()

    The \c loadedChanged signal is emitted when the loading state has changed.
*/
/*!
    \qmlsignal QtMultimedia::SoundEffect::loadedChanged()

    The \c loadedChanged signal is emitted when the loading state has changed.
*/

/*!
    \fn void QSoundEffect::loopCountChanged()

    The \c loopCountChanged signal is emitted when the initial number of loops has changed.
*/
/*!
    \qmlsignal QtMultimedia::SoundEffect::loopCountChanged()

    The \c loopCountChanged signal is emitted when the initial number of loops has changed.
*/

/*!
    \fn void QSoundEffect::loopsRemainingChanged()

    The \c loopsRemainingChanged signal is emitted when the remaining number of loops has changed.
*/
/*!
    \qmlsignal QtMultimedia::SoundEffect::loopsRemainingChanged()

    The \c loopsRemainingChanged signal is emitted when the remaining number of loops has changed.
*/

/*!
    \fn void QSoundEffect::volumeChanged()

    The \c volumeChanged signal is emitted when the volume has changed.
*/
/*!
    \qmlsignal QtMultimedia::SoundEffect::volumeChanged()

    The \c volumeChanged signal is emitted when the volume has changed.
*/

/*!
    \fn void QSoundEffect::mutedChanged()

    The \c mutedChanged signal is emitted when the mute state has changed.
*/
/*!
    \qmlsignal QtMultimedia::SoundEffect::mutedChanged()

    The \c mutedChanged signal is emitted when the mute state has changed.
*/

/*!
    \fn void QSoundEffect::playingChanged()

    The \c playingChanged signal is emitted when the playing property has changed.
*/
/*!
    \qmlsignal QtMultimedia::SoundEffect::playingChanged()

    The \c playingChanged signal is emitted when the playing property has changed.
*/

/*!
    \fn void QSoundEffect::statusChanged()

    The \c statusChanged signal is emitted when the status property has changed.
*/
/*!
    \qmlsignal QtMultimedia::SoundEffect::statusChanged()

    The \c statusChanged signal is emitted when the status property has changed.
*/

QSoundEffectPrivate *QSoundEffectPrivate::get(QSoundEffect *sfx)
{
    return sfx->d_func();
}

QT_END_NAMESPACE

#include "moc_qsoundeffect.cpp"

/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "DebugMacros.h"

#include "s60audioplayersession.h"
#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>

/*!
    Constructs the CMdaAudioPlayerUtility object with given \a parent QObject.

    And Registers for Audio Loading Notifications.

*/

S60AudioPlayerSession::S60AudioPlayerSession(QObject *parent)
    : S60MediaPlayerSession(parent)
    , m_player(0)
    , m_audioEndpoint("Default")
{
    DP0("S60AudioPlayerSession::S60AudioPlayerSession +++");

#ifdef HAS_AUDIOROUTING
    m_audioOutput = 0;
#endif //HAS_AUDIOROUTING    
    QT_TRAP_THROWING(m_player = CAudioPlayer::NewL(*this, 0, EMdaPriorityPreferenceNone));
    m_player->RegisterForAudioLoadingNotification(*this);

    DP0("S60AudioPlayerSession::S60AudioPlayerSession ---");
}


/*!
   Destroys the CMdaAudioPlayerUtility object.

    And Unregister the observer.

*/

S60AudioPlayerSession::~S60AudioPlayerSession()
{
    DP0("S60AudioPlayerSession::~S60AudioPlayerSession +++");
#ifdef HAS_AUDIOROUTING
    if (m_audioOutput)
        m_audioOutput->UnregisterObserver(*this);
    delete m_audioOutput;
#endif
    m_player->Close();
    delete m_player;

    DP0("S60AudioPlayerSession::~S60AudioPlayerSession ---");
}

/*!

   Opens the a file from \a path.

*/

void S60AudioPlayerSession::doLoadL(const TDesC &path)
{
    DP0("S60AudioPlayerSession::doLoadL +++");

#ifdef HAS_AUDIOROUTING
    // m_audioOutput needs to be reinitialized after MapcInitComplete
    if (m_audioOutput)
        m_audioOutput->UnregisterObserver(*this);
    delete m_audioOutput;
    m_audioOutput = NULL;
#endif //HAS_AUDIOROUTING
    m_player->OpenFileL(path);

    DP0("S60AudioPlayerSession::doLoadL ---");
}

/*!

  Returns the duration of the audio sample in microseconds.

*/

qint64 S60AudioPlayerSession::doGetDurationL() const
{
 //  DP0("S60AudioPlayerSession::doGetDurationL");

    return m_player->Duration().Int64() / qint64(1000);
}

/*!
 *  Returns the current playback position in microseconds from the start of the clip.

*/

qint64 S60AudioPlayerSession::doGetPositionL() const
{
  //  DP0("S60AudioPlayerSession::doGetPositionL");

    TTimeIntervalMicroSeconds ms = 0;
    m_player->GetPosition(ms);
    return ms.Int64() / qint64(1000);
}

/*!
   Returns TRUE if Video available or else FALSE
 */

bool S60AudioPlayerSession::isVideoAvailable()
{
    DP0("S60AudioPlayerSession::isVideoAvailable");

    return false;
}

/*!
   Returns TRUE if Audio available or else FALSE
 */
bool S60AudioPlayerSession::isAudioAvailable()
{
    DP0("S60AudioPlayerSession::isAudioAvailable");

    return true; // this is a bit happy scenario, but we do emit error that we can't play
}

/*!
   Starts loading Media and sets media status to Buffering.

 */

void S60AudioPlayerSession::MaloLoadingStarted()
{
    DP0("S60AudioPlayerSession::MaloLoadingStarted +++");

    buffering();

    DP0("S60AudioPlayerSession::MaloLoadingStarted ---");
}


/*!
   Indicates loading Media is completed.

   And sets media status to Buffered.

 */

void S60AudioPlayerSession::MaloLoadingComplete()
{
    DP0("S60AudioPlayerSession::MaloLoadingComplete +++");

    buffered();

    DP0("S60AudioPlayerSession::MaloLoadingComplete ---");
}

/*!
    Start or resume playing the current source.
*/

void S60AudioPlayerSession::doPlay()
{
    DP0("S60AudioPlayerSession::doPlay +++");

    // For some reason loading progress callback are not called on emulator
    // Same is the case with hardware. Will be fixed as part of QTMOBILITY-782.
        
    //#ifdef __WINSCW__
        buffering();
    //#endif
        m_player->Play();
    //#ifdef __WINSCW__
        buffered();
    //#endif

    DP0("S60AudioPlayerSession::doPlay ---");
}


/*!
    Pause playing the current source.
*/


void S60AudioPlayerSession::doPauseL()
{
    DP0("S60AudioPlayerSession::doPauseL +++");

    m_player->Pause();

    DP0("S60AudioPlayerSession::doPauseL ---");
}


/*!

    Stop playing, and reset the play position to the beginning.
*/

void S60AudioPlayerSession::doStop()
{
    DP0("S60AudioPlayerSession::doStop +++");

    m_player->Stop();

    DP0("S60AudioPlayerSession::doStop ---");
}

/*!
    Closes the current audio clip (allowing another clip to be opened)
*/

void S60AudioPlayerSession::doClose()
{
    DP0("S60AudioPlayerSession::doClose +++");

#ifdef HAS_AUDIOROUTING
    if (m_audioOutput) {
        m_audioOutput->UnregisterObserver(*this);
        delete m_audioOutput;
        m_audioOutput = NULL;
    }
#endif
    m_player->Close();

    DP0("S60AudioPlayerSession::doClose ---");
}

/*!

    Changes the current playback volume to specified \a value.
*/

void S60AudioPlayerSession::doSetVolumeL(int volume)
{
    DP0("S60AudioPlayerSession::doSetVolumeL +++");

    DP1("S60AudioPlayerSession::doSetVolumeL, Volume:", volume);

    m_player->SetVolume(volume * m_player->MaxVolume() / 100);

    DP0("S60AudioPlayerSession::doSetVolumeL ---");
}

/*!
    Sets the current playback position to \a microSeconds from the start of the clip.
*/

void S60AudioPlayerSession::doSetPositionL(qint64 microSeconds)
{
    DP0("S60AudioPlayerSession::doSetPositionL +++");

    DP1("S60AudioPlayerSession::doSetPositionL, Microseconds:", microSeconds);

    m_player->SetPosition(TTimeIntervalMicroSeconds(microSeconds));

    DP0("S60AudioPlayerSession::doSetPositionL ---");
}

/*!

    Updates meta data entries in the current audio clip.
*/

void S60AudioPlayerSession::updateMetaDataEntriesL()
{
    DP0("S60AudioPlayerSession::updateMetaDataEntriesL +++");

    metaDataEntries().clear();
    int numberOfMetaDataEntries = 0;

    //User::LeaveIfError(m_player->GetNumberOfMetaDataEntries(numberOfMetaDataEntries));
    m_player->GetNumberOfMetaDataEntries(numberOfMetaDataEntries);

    for (int i = 0; i < numberOfMetaDataEntries; i++) {
        CMMFMetaDataEntry *entry = NULL;
        entry = m_player->GetMetaDataEntryL(i);
        metaDataEntries().insert(QString::fromUtf16(entry->Name().Ptr(), entry->Name().Length()), QString::fromUtf16(entry->Value().Ptr(), entry->Value().Length()));
        delete entry;
    }
    emit metaDataChanged();

    DP0("S60AudioPlayerSession::updateMetaDataEntriesL ---");
}

/*!
    Sets the playbackRate with \a rate.
*/

void S60AudioPlayerSession::setPlaybackRate(qreal rate)
{
    DP0("S60AudioPlayerSession::setPlaybackRate +++");
    DP1("S60AudioPlayerSession::setPlaybackRate, Rate:", rate);
    /*Since AudioPlayerUtility doesn't support set playback rate hence
     * setPlaybackRate emits playbackRateChanged signal for 1.0x ie normal playback.
     * For all other playBackRates it sets and emits error signal.
    */
    if (rate == 1.0) {
        emit playbackRateChanged(rate);
        return;
    } else {
        int err = KErrNotSupported;
        setAndEmitError(err);
    }
    DP0("S60AudioPlayerSession::setPlaybackRate ---");
}

/*!

    Returns the percentage of the audio clip loaded.
*/

int S60AudioPlayerSession::doGetBufferStatusL() const
{
    DP0("S60AudioPlayerSession::doGetBufferStatusL +++");

    int progress = 0;
    m_player->GetAudioLoadingProgressL(progress);

    DP0("S60AudioPlayerSession::doGetBufferStatusL ---");

    return progress;
}

/*!

    Defines required client behaviour when an attempt to open and initialise an audio sample has completed,
    successfully or not.

    \a aError if KErrNone the sample is ready to play or else system wide error.

    \a aDuration The duration of the audio sample.
*/

#ifdef S60_DRM_SUPPORTED
void S60AudioPlayerSession::MdapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration)
#else
void S60AudioPlayerSession::MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration)
#endif
{
    DP0("S60AudioPlayerSession::MdapcInitComplete +++");

    DP1("S60AudioPlayerSession::MdapcInitComplete - aError", aError);

    Q_UNUSED(aDuration);
    setError(aError);
    if (KErrNone != aError)
        return;
#ifdef HAS_AUDIOROUTING    
    TRAPD(err, 
        m_audioOutput = CAudioOutput::NewL(*m_player);
        m_audioOutput->RegisterObserverL(*this);
    );
    setActiveEndpoint(m_audioEndpoint);
    setError(err);
#endif //HAS_AUDIOROUTING
    if (KErrNone == aError)
        loaded();

    DP0("S60AudioPlayerSession::MdapcInitComplete ---");
}

/*!
    Defines required client behaviour when an attempt to playback an audio sample has completed,
    successfully or not.

    \a aError if KErrNone the playback completed or else system wide error.
*/


#ifdef S60_DRM_SUPPORTED
void S60AudioPlayerSession::MdapcPlayComplete(TInt aError)
#else
void S60AudioPlayerSession::MapcPlayComplete(TInt aError)
#endif
{
    DP0("S60AudioPlayerSession::MdapcPlayComplete +++");

    DP1("S60AudioPlayerSession::MdapcPlayComplete", aError);

    if (KErrNone == aError)
        endOfMedia();
    else
        setError(aError);

    DP0("S60AudioPlayerSession::MdapcPlayComplete ---");
}

/*!
    Defiens which Audio End point to use.

    \a audioEndpoint audioEndpoint name.
*/

void S60AudioPlayerSession::doSetAudioEndpoint(const QString& audioEndpoint)
{
    DP0("S60AudioPlayerSession::doSetAudioEndpoint +++");

    DP1("S60AudioPlayerSession::doSetAudioEndpoint - ", audioEndpoint);

    m_audioEndpoint = audioEndpoint;

    DP0("S60AudioPlayerSession::doSetAudioEndpoint ---");
}

/*!

    Returns audioEndpoint name.
*/

QString S60AudioPlayerSession::activeEndpoint() const
{
    DP0("S60AudioPlayerSession::activeEndpoint +++");

    QString outputName = QString("Default");
#ifdef HAS_AUDIOROUTING
    if (m_audioOutput) {
        CAudioOutput::TAudioOutputPreference output = m_audioOutput->AudioOutput();
        outputName = qStringFromTAudioOutputPreference(output);
    }
#endif
    DP1("S60AudioPlayerSession::activeEndpoint is :", outputName);

    DP0("S60AudioPlayerSession::activeEndpoint ---");
    return outputName;
}

/*!
 *  Returns default Audio End point in use.
*/

QString S60AudioPlayerSession::defaultEndpoint() const
{
    DP0("S60AudioPlayerSession::defaultEndpoint +++");

    QString outputName = QString("Default");
#ifdef HAS_AUDIOROUTING
    if (m_audioOutput) {
        CAudioOutput::TAudioOutputPreference output = m_audioOutput->DefaultAudioOutput();
        outputName = qStringFromTAudioOutputPreference(output);
    }
#endif
    DP1("S60AudioPlayerSession::defaultEndpoint is :", outputName);

    DP0("S60AudioPlayerSession::defaultEndpoint ---");
    return outputName;
}

/*!
   Sets active end \a name as an Audio End point.
*/

void S60AudioPlayerSession::setActiveEndpoint(const QString& name)
{
    DP0("S60AudioPlayerSession::setActiveEndpoint +++");

    DP1("S60AudioPlayerSession::setActiveEndpoint - ", name);

#ifdef HAS_AUDIOROUTING
    CAudioOutput::TAudioOutputPreference output = CAudioOutput::ENoPreference;

    if (name == QString("Default"))
        output = CAudioOutput::ENoPreference;
    else if (name == QString("All"))
        output = CAudioOutput::EAll;
    else if (name == QString("None"))
        output = CAudioOutput::ENoOutput;
    else if (name == QString("Earphone"))
        output = CAudioOutput::EPrivate;
    else if (name == QString("Speaker"))
        output = CAudioOutput::EPublic;

    if (m_audioOutput) {
        TRAPD(err, m_audioOutput->SetAudioOutputL(output));
        setError(err);
    }
#endif

    DP0("S60AudioPlayerSession::setActiveEndpoint ---");
}

/*!
    The default audio output has been changed.

    \a aAudioOutput Audio Output object.

    \a aNewDefault is CAudioOutput::TAudioOutputPreference.
*/


#ifdef HAS_AUDIOROUTING
void S60AudioPlayerSession::DefaultAudioOutputChanged(CAudioOutput& aAudioOutput,
                                        CAudioOutput::TAudioOutputPreference aNewDefault)
{
    DP0("S60AudioPlayerSession::DefaultAudioOutputChanged +++");

    // Emit already implemented in setActiveEndpoint function
    Q_UNUSED(aAudioOutput)
    Q_UNUSED(aNewDefault)

    DP0("S60AudioPlayerSession::DefaultAudioOutputChanged ---");
}


/*!
    Converts CAudioOutput::TAudioOutputPreference enum to QString.

    \a output is  CAudioOutput::TAudioOutputPreference enum value.

*/

QString S60AudioPlayerSession::qStringFromTAudioOutputPreference(CAudioOutput::TAudioOutputPreference output) const
{
    DP0("S60AudioPlayerSession::qStringFromTAudioOutputPreference");

    if (output == CAudioOutput::ENoPreference)
        return QString("Default");
    else if (output == CAudioOutput::EAll)
        return QString("All");
    else if (output == CAudioOutput::ENoOutput)
        return QString("None");
    else if (output == CAudioOutput::EPrivate)
        return QString("Earphone");
    else if (output == CAudioOutput::EPublic)
        return QString("Speaker");
    return QString("Default");
}
#endif

/*!
    Return True if its Seekable or else False.
*/

bool S60AudioPlayerSession::getIsSeekable() const
{
    DP0("S60AudioPlayerSession::getIsSeekable");

    return ETrue;
}


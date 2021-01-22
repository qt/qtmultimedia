/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qaudioencodersettingscontrol.h"
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE


/*!
    \class QAudioEncoderSettingsControl
    \obsolete
    \inmodule QtMultimedia

    \ingroup multimedia_control

    \brief The QAudioEncoderSettingsControl class provides access to the settings of a
    media service that performs audio encoding.

    If a QMediaService supports encoding audio data it will implement
    QAudioEncoderSettingsControl.  This control provides information about the limits
    of restricted audio encoder options and allows the selection of a set of
    audio encoder settings as specified in a QAudioEncoderSettings object.

    The functionality provided by this control is exposed to application code through the
    QMediaRecorder class.

    The interface name of QAudioEncoderSettingsControl is \c org.qt-project.qt.audioencodersettingscontrol/5.0 as
    defined in QAudioEncoderSettingsControl_iid.

    \sa QMediaService::requestControl(), QMediaRecorder
*/

/*!
    \macro QAudioEncoderSettingsControl_iid

    \c org.qt-project.qt.audioencodersettingscontrol/5.0

    Defines the interface name of the QAudioEncoderSettingsControl class.

    \relates QAudioEncoderSettingsControl
*/

/*!
  Create a new audio encoder settings control object with the given \a parent.
*/
QAudioEncoderSettingsControl::QAudioEncoderSettingsControl(QObject *parent)
    :QMediaControl(parent)
{
}

/*!
  Destroys the audio encoder settings control.
*/
QAudioEncoderSettingsControl::~QAudioEncoderSettingsControl()
{
}

/*!
  \fn QAudioEncoderSettingsControl::supportedAudioCodecs() const

  Returns the list of supported audio codec names.
*/

/*!
  \fn QAudioEncoderSettingsControl::codecDescription(const QString &codecName) const

  Returns the description of audio codec \a codecName.
*/

/*!
  \fn QAudioEncoderSettingsControl::supportedSampleRates(const QAudioEncoderSettings &settings = QAudioEncoderSettings(),
                                                 bool *continuous) const

  Returns the list of supported audio sample rates, if known.

  If non null audio \a settings parameter is passed,
  the returned list is reduced to sample rates supported with partial settings applied.

  It can be used for example to query the list of sample rates, supported by specific audio codec.

  If the encoder supports arbitrary sample rates within the supported rates range,
  *\a continuous is set to true, otherwise *\a continuous is set to false.
*/

/*!
    \fn QAudioEncoderSettingsControl::audioSettings() const

    Returns the audio encoder settings.

    The returned value may be different tha passed to QAudioEncoderSettingsControl::setAudioSettings()
    if the settings contains the default or undefined parameters.
    In this case if the undefined parameters are already resolved, they should be returned.
*/

/*!
    \fn QAudioEncoderSettingsControl::setAudioSettings(const QAudioEncoderSettings &settings)

    Sets the selected audio \a settings.
*/

QT_END_NAMESPACE

#include "moc_qaudioencodersettingscontrol.cpp"

/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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


#include "qaudiosystemplugin.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAudioSystemPlugin
    \brief The QAudioSystemPlugin class provides an abstract base for audio plugins.

    \ingroup multimedia
    \ingroup multimedia_audio
    \inmodule QtMultimedia
    \internal

    Writing a audio plugin is achieved by subclassing this base class,
    reimplementing the pure virtual functions keys(), availableDevices(),
    createInput(), createOutput() and createDeviceInfo() then exporting
    the class with the Q_EXPORT_PLUGIN2() macro.

    Unit tests are available to help in debugging new plugins.

    \sa QAbstractAudioDeviceInfo, QAbstractAudioOutput, QAbstractAudioInput

    Qt supports win32, linux(alsa) and Mac OS X standard (builtin to the
    QtMultimedia library at compile time).

    You can support other backends other than these predefined ones by
    creating a plugin subclassing QAudioSystemPlugin, QAbstractAudioDeviceInfo,
    QAbstractAudioOutput and QAbstractAudioInput.

    Add "default" to your list of keys() available to override the default
    audio device to be provided by your plugin.

    -audio-backend configure option will force compiling in of the builtin backend
    into the QtMultimedia library at compile time. This is automatic by default
    and will only be compiled into the library if the dependencies are installed.
    eg. alsa-devel package installed for linux.

    If the builtin backend is not compiled into the QtMultimedia library and
    no audio plugins are available a fallback dummy backend will be used.
    This should print out warnings if this is the case when you try and use QAudioInput or QAudioOutput. To fix this problem
    reconfigure Qt using -audio-backend or create your own plugin with a default
    key to always override the dummy fallback. The easiest way to determine
    if you have only a dummy backend is to get a list of available audio devices.

    QAudioDeviceInfo::availableDevices(QAudio::AudioOutput).size() = 0 (dummy backend)
*/

/*!
    Construct a new audio plugin with \a parent.
    This is invoked automatically by the Q_EXPORT_PLUGIN2() macro.
*/

QAudioSystemPlugin::QAudioSystemPlugin(QObject* parent) :
    QObject(parent)
{}

/*!
   Destroy the audio plugin

   You never have to call this explicitly. Qt destroys a plugin automatically when it is no longer used.
*/

QAudioSystemPlugin::~QAudioSystemPlugin()
{}

/*!
    \fn QStringList QAudioSystemPlugin::keys() const
    Returns the list of device identifiers this plugin supports.
*/

/*!
    \fn QList<QByteArray> QAudioSystemPlugin::availableDevices(QAudio::Mode mode) const
    Returns a list of available audio devices for \a mode
*/

/*!
    \fn QAbstractAudioInput* QAudioSystemPlugin::createInput(const QByteArray& device)
    Returns a pointer to a QAbstractAudioInput created using \a device identifier
*/

/*!
    \fn QAbstractAudioOutput* QAudioSystemPlugin::createOutput(const QByteArray& device)
    Returns a pointer to a QAbstractAudioOutput created using \a device identifier

*/

/*!
    \fn QAbstractAudioDeviceInfo* QAudioSystemPlugin::createDeviceInfo(const QByteArray& device, QAudio::Mode mode)
    Returns a pointer to a QAbstractAudioDeviceInfo created using \a device and \a mode

*/


QT_END_NAMESPACE

#include "moc_qaudiosystemplugin.cpp"

/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
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
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwasapiplugin.h"
#include "qwasapiaudiodeviceinfo.h"
#include "qwasapiaudioinput.h"
#include "qwasapiaudiooutput.h"
#include "qwasapiutils.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcMmPlugin, "qt.multimedia.plugin")

QWasapiPlugin::QWasapiPlugin(QObject *parent)
    : QAudioSystemPlugin(parent)
{
    qCDebug(lcMmPlugin) << __FUNCTION__;
}

QList<QByteArray> QWasapiPlugin::availableDevices(QAudio::Mode mode) const
{
    qCDebug(lcMmPlugin) << __FUNCTION__ << mode;
    return QWasapiUtils::availableDevices(mode);
}

QAbstractAudioInput *QWasapiPlugin::createInput(const QByteArray &device)
{
    qCDebug(lcMmPlugin) << __FUNCTION__ << device;
    return new QWasapiAudioInput(device);
}

QAbstractAudioOutput *QWasapiPlugin::createOutput(const QByteArray &device)
{
    qCDebug(lcMmPlugin) << __FUNCTION__ << device;
    return new QWasapiAudioOutput(device);
}

QAbstractAudioDeviceInfo *QWasapiPlugin::createDeviceInfo(const QByteArray &device, QAudio::Mode mode)
{
    qCDebug(lcMmPlugin) << __FUNCTION__ << device << mode;
    return new QWasapiAudioDeviceInfo(device, mode);
}

QT_END_NAMESPACE

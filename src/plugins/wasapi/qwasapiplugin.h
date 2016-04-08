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

#ifndef QWASAPIPLUGIN_H
#define QWASAPIPLUGIN_H

#include <QtCore/QLoggingCategory>
#include <QtCore/QList>
#include <QtMultimedia/QAudioSystemPlugin>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcMmPlugin)

class QWasapiPlugin : public QAudioSystemPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.qt-project.qt.audiosystemfactory/5.0" FILE "wasapi.json")

public:
    explicit QWasapiPlugin(QObject *parent = 0);
    ~QWasapiPlugin() {}

    QList<QByteArray> availableDevices(QAudio::Mode mode) const Q_DECL_OVERRIDE;
    QAbstractAudioInput *createInput(const QByteArray &device) Q_DECL_OVERRIDE;
    QAbstractAudioOutput *createOutput(const QByteArray &device) Q_DECL_OVERRIDE;
    QAbstractAudioDeviceInfo *createDeviceInfo(const QByteArray &device, QAudio::Mode mode) Q_DECL_OVERRIDE;

private:
    QList<QByteArray> m_deviceNames;
    QList<QByteArray> m_deviceIds;
};

QT_END_NAMESPACE

#endif // QWASAPIPLUGIN_H

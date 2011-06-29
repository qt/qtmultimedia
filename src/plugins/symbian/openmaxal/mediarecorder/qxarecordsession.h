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

#ifndef QXARECORDSESSION_H
#define QXARECORDSESSION_H

#include <QObject>
#include <QUrl>
#include "qmediarecorder.h"
#include "xarecordsessioncommon.h"

QT_USE_NAMESPACE

class XARecordSessionImpl;

/*
 * This is a backend class for all QXXXControl objects.
 * This class contains actual implementation of recording functionality
 * from the control object's perspective.
 */


class QXARecordSession : public QObject,
                         public XARecordObserver
{
Q_OBJECT

public:
    QXARecordSession(QObject *parent);
    virtual ~QXARecordSession();

    /* For QMediaRecorderControl begin */
    QUrl outputLocation();
    bool setOutputLocation(const QUrl &location);
    QMediaRecorder::State state();
    qint64 duration();
    void applySettings();

    void record();
    void pause();
    void stop();

    void cbDurationChanged(TInt64 new_pos);
    void cbAvailableAudioInputsChanged();
    void cbRecordingStarted();
    void cbRecordingStopped();
    /* For QMediaRecorderControl end */

    /* For QAudioEndpointSelector begin */
    QList<QString> availableEndpoints();
    QString endpointDescription(const QString &name);
    QString defaultEndpoint();
    QString activeEndpoint();
    void setActiveEndpoint(const QString &name);
    /* For QAudioEndpointSelector end */

    /* For QAudioEncoderControl begin */
    QStringList supportedAudioCodecs();
    QString codecDescription(const QString &codecName);
    QList<int> supportedSampleRates(
                const QAudioEncoderSettings &settings,
                bool *continuous);
    QAudioEncoderSettings audioSettings();
    void setAudioSettings(const QAudioEncoderSettings &settings);
    QStringList supportedEncodingOptions(const QString &codec);
    QVariant encodingOption(const QString &codec, const QString &name);
    void setEncodingOption(const QString &codec, const QString &name, const QVariant &value);
    /* For QAudioEncoderControl end */

    /* For QMediaContainerControl begin */
    QStringList supportedContainers();
    QString containerMimeType();
    void setContainerMimeType(const QString &formatMimeType);
    QString containerDescription(const QString &formatMimeType);
    /* For QMediaContainerControl end */

Q_SIGNALS:
    void stateChanged(QMediaRecorder::State state);
    void durationChanged(qint64 duration);
    void error(int error, const QString &errorString);
    void activeEndpointChanged(const QString& name);
    void availableAudioInputsChanged();

private:
    void setRecorderState(QMediaRecorder::State state);
    void initCodecsList();
    void initContainersList();
    bool setEncoderSettingsToImpl();
    bool setURIToImpl();

private:
    /* Own */
    XARecordSessionImpl *m_impl;
    QUrl m_outputLocation;
    bool m_URItoImplSet;
    QMediaRecorder::State m_state;
    QMediaRecorder::State m_previousState;
    QStringList m_codecs;
    QAudioEncoderSettings m_audioencodersettings;
    QAudioEncoderSettings m_appliedaudioencodersettings;
    QStringList m_containers;
    QStringList m_containersDesc;
    QString m_containerMimeType;
};

#endif /* QXARECORDSESSION_H */

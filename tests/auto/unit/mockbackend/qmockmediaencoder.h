/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKRECORDERCONTROL_H
#define MOCKRECORDERCONTROL_H

#include <QUrl>
#include <qaudiodevice.h>

#include "private/qplatformmediaencoder_p.h"

class QMockMediaEncoder : public QPlatformMediaEncoder
{
public:
    QMockMediaEncoder(QMediaRecorder *parent):
        QPlatformMediaEncoder(parent),
        m_state(QMediaRecorder::StoppedState),
        m_position(0),
        m_settingAppliedCount(0)
    {
    }

    bool isLocationWritable(const QUrl &) const
    {
        return true;
    }

    QMediaRecorder::RecorderState state() const
    {
        return m_state;
    }

    qint64 duration() const
    {
        return m_position;
    }

    void applySettings(const QMediaEncoderSettings &settings)
    {
        m_settings = settings;
        m_settingAppliedCount++;
    }

    virtual void setMetaData(const QMediaMetaData &m)
    {
        m_metaData = m;
        emit metaDataChanged();
    }
    virtual QMediaMetaData metaData() const { return m_metaData; }

    using QPlatformMediaEncoder::error;

public:
    void record(const QMediaEncoderSettings &)
    {
        m_state = QMediaRecorder::RecordingState;
        m_position=1;
        emit stateChanged(m_state);
        emit durationChanged(m_position);

        QUrl actualLocation = outputLocation().isEmpty() ? QUrl::fromLocalFile("default_name.mp4") : outputLocation();
        emit actualLocationChanged(actualLocation);
    }

    void pause()
    {
        m_state = QMediaRecorder::PausedState;
        emit stateChanged(m_state);
    }

    void resume()
    {
        m_state = QMediaRecorder::RecordingState;
        emit stateChanged(m_state);
    }

    void stop()
    {
        m_position=0;
        m_state = QMediaRecorder::StoppedState;
        emit stateChanged(m_state);
    }

public:
    QMediaMetaData m_metaData;
    QMediaRecorder::RecorderState m_state;
    QMediaEncoderSettings m_settings;
    qint64     m_position;
    int m_settingAppliedCount;
};

#endif // MOCKRECORDERCONTROL_H

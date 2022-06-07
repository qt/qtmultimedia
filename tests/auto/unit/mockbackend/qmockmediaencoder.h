// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MOCKRECORDERCONTROL_H
#define MOCKRECORDERCONTROL_H

#include <QUrl>
#include <qaudiodevice.h>

#include "private/qplatformmediarecorder_p.h"

class QMockMediaEncoder : public QPlatformMediaRecorder
{
public:
    QMockMediaEncoder(QMediaRecorder *parent):
        QPlatformMediaRecorder(parent),
        m_state(QMediaRecorder::StoppedState),
        m_position(0)
    {
    }

    bool isLocationWritable(const QUrl &) const override
    {
        return true;
    }

    QMediaRecorder::RecorderState state() const override
    {
        return m_state;
    }

    qint64 duration() const override
    {
        return m_position;
    }

    virtual void setMetaData(const QMediaMetaData &m) override
    {
        m_metaData = m;
        emit metaDataChanged();
    }
    virtual QMediaMetaData metaData() const override { return m_metaData; }

    using QPlatformMediaRecorder::error;

public:
    void record(QMediaEncoderSettings &settings) override
    {
        m_state = QMediaRecorder::RecordingState;
        m_settings = settings;
        m_position=1;
        emit stateChanged(m_state);
        emit durationChanged(m_position);

        QUrl actualLocation = outputLocation().isEmpty() ? QUrl::fromLocalFile("default_name.mp4") : outputLocation();
        emit actualLocationChanged(actualLocation);
    }

    void pause() override
    {
        m_state = QMediaRecorder::PausedState;
        emit stateChanged(m_state);
    }

    void resume() override
    {
        m_state = QMediaRecorder::RecordingState;
        emit stateChanged(m_state);
    }

    void stop() override
    {
        m_position=0;
        m_state = QMediaRecorder::StoppedState;
        emit stateChanged(m_state);
    }

    void reset()
    {
        m_state = QMediaRecorder::StoppedState;
        m_settings = QMediaEncoderSettings();
        m_position = 0;
        emit stateChanged(m_state);
        emit durationChanged(m_position);
        clearActualLocation();
    }

public:
    QMediaMetaData m_metaData;
    QMediaRecorder::RecorderState m_state;
    QMediaEncoderSettings m_settings;
    qint64     m_position;
};

#endif // MOCKRECORDERCONTROL_H

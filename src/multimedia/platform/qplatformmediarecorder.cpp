// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformmediarecorder_p.h"
#include <QObject>

QT_BEGIN_NAMESPACE

QPlatformMediaRecorder::QPlatformMediaRecorder(QMediaRecorder *parent)
    : q(parent)
{
}

void QPlatformMediaRecorder::pause()
{
    error(QMediaRecorder::FormatError, QMediaRecorder::tr("Pause not supported"));
}

void QPlatformMediaRecorder::resume()
{
    error(QMediaRecorder::FormatError, QMediaRecorder::tr("Resume not supported"));
}

void QPlatformMediaRecorder::stateChanged(QMediaRecorder::RecorderState state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit q->recorderStateChanged(state);
}

void QPlatformMediaRecorder::durationChanged(qint64 duration)
{
    if (m_duration == duration)
        return;
    m_duration = duration;
    emit q->durationChanged(duration);
}

void QPlatformMediaRecorder::actualLocationChanged(const QUrl &location)
{
    if (m_actualLocation == location)
        return;
    m_actualLocation = location;
    emit q->actualLocationChanged(location);
}

void QPlatformMediaRecorder::error(QMediaRecorder::Error error, const QString &errorString)
{
    m_error.setAndNotify(error, errorString, *q);
}

void QPlatformMediaRecorder::metaDataChanged()
{
    emit q->metaDataChanged();
}

QT_END_NAMESPACE

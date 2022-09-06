// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QWidget>

/**
 * Widget which displays a the current fill state of the Engine's internal
 * buffer, and the current play/record position within that buffer.
 */
class ProgressBar : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressBar(QWidget *parent = nullptr);
    ~ProgressBar();

    void reset();
    void paintEvent(QPaintEvent *event) override;

public slots:
    void bufferLengthChanged(qint64 length);
    void recordPositionChanged(qint64 recordPosition);
    void playPositionChanged(qint64 playPosition);
    void windowChanged(qint64 position, qint64 length);

private:
    qint64 m_bufferLength;
    qint64 m_recordPosition;
    qint64 m_playPosition;
    qint64 m_windowPosition;
    qint64 m_windowLength;
};

#endif // PROGRESSBAR_H

// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SPECTROGRAPH_H
#define SPECTROGRAPH_H

#include "frequencyspectrum.h"

#include <QWidget>

/**
 * Widget which displays a spectrograph showing the frequency spectrum
 * of the window of audio samples most recently analyzed by the Engine.
 */
class Spectrograph : public QWidget
{
    Q_OBJECT

public:
    explicit Spectrograph(QWidget *parent = nullptr);
    ~Spectrograph();

    void setParams(int numBars, qreal lowFreq, qreal highFreq);

    // QObject
    void timerEvent(QTimerEvent *event) override;

    // QWidget
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void infoMessage(const QString &message, int intervalMs);

public slots:
    void reset();
    void spectrumChanged(const FrequencySpectrum &spectrum);

private:
    int barIndex(qreal frequency) const;
    QPair<qreal, qreal> barRange(int barIndex) const;
    void updateBars();

    void selectBar(int index);

private:
    struct Bar
    {
        Bar() : value(0.0), clipped(false) { }
        qreal value;
        bool clipped;
    };

    QList<Bar> m_bars;
    int m_barSelected;
    int m_timerId;
    qreal m_lowFreq;
    qreal m_highFreq;
    FrequencySpectrum m_spectrum;
};

#endif // SPECTROGRAPH_H

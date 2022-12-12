// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "levelmeter.h"

#include <QDebug>
#include <QPainter>
#include <QTimer>

#include <math.h>

// Constants
const int RedrawInterval = 100; // ms
const qreal PeakDecayRate = 0.001;
const int PeakHoldLevelDuration = 2000; // ms

LevelMeter::LevelMeter(QWidget *parent)
    : QWidget(parent),
      m_rmsLevel(0.0),
      m_peakLevel(0.0),
      m_decayedPeakLevel(0.0),
      m_peakDecayRate(PeakDecayRate),
      m_peakHoldLevel(0.0),
      m_redrawTimer(new QTimer(this)),
      m_rmsColor(Qt::red),
      m_peakColor(255, 200, 200, 255)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    setMinimumWidth(30);

    connect(m_redrawTimer, &QTimer::timeout, this, &LevelMeter::redrawTimerExpired);
    m_redrawTimer->start(RedrawInterval);
}

LevelMeter::~LevelMeter() = default;

void LevelMeter::reset()
{
    m_rmsLevel = 0.0;
    m_peakLevel = 0.0;
    update();
}

void LevelMeter::levelChanged(qreal rmsLevel, qreal peakLevel, int numSamples)
{
    // Smooth the RMS signal
    const qreal smooth =
            pow(qreal(0.9), static_cast<qreal>(numSamples) / 256); // TODO: remove this magic number
    m_rmsLevel = (m_rmsLevel * smooth) + (rmsLevel * (1.0 - smooth));

    if (peakLevel > m_decayedPeakLevel) {
        m_peakLevel = peakLevel;
        m_decayedPeakLevel = peakLevel;
        m_peakLevelChanged.start();
    }

    if (peakLevel > m_peakHoldLevel) {
        m_peakHoldLevel = peakLevel;
        m_peakHoldLevelChanged.start();
    }

    update();
}

void LevelMeter::redrawTimerExpired()
{
    // Decay the peak signal
    const int elapsedMs = m_peakLevelChanged.elapsed();
    const qreal decayAmount = m_peakDecayRate * elapsedMs;
    if (decayAmount < m_peakLevel)
        m_decayedPeakLevel = m_peakLevel - decayAmount;
    else
        m_decayedPeakLevel = 0.0;

    // Check whether to clear the peak hold level
    if (m_peakHoldLevelChanged.elapsed() > PeakHoldLevelDuration)
        m_peakHoldLevel = 0.0;

    update();
}

void LevelMeter::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    QRect bar = rect();

    bar.setTop(rect().top() + (1.0 - m_peakHoldLevel) * rect().height());
    bar.setBottom(bar.top() + 5);
    painter.fillRect(bar, m_rmsColor);
    bar.setBottom(rect().bottom());

    bar.setTop(rect().top() + (1.0 - m_decayedPeakLevel) * rect().height());
    painter.fillRect(bar, m_peakColor);

    bar.setTop(rect().top() + (1.0 - m_rmsLevel) * rect().height());
    painter.fillRect(bar, m_rmsColor);
}

#include "moc_levelmeter.cpp"

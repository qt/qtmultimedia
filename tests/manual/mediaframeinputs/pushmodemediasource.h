// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PUSHMODEMEDIASOURCE_H
#define PUSHMODEMEDIASOURCE_H

#include <QTimer>
#include <QtNumeric>

template <typename Generator>
class PushModeFrameSource
{
public:
    PushModeFrameSource(const typename Generator::Settings &generatorSettings, qreal producingRate)
        : m_generator(generatorSettings),
          m_timerInterval(std::chrono::duration_cast<std::chrono::milliseconds>(
                  m_generator.interval() / producingRate))
    {
        m_timer.setTimerType(Qt::PreciseTimer);
        m_timer.setInterval(0);
    }

    template <typename... Callback>
    void addFrameReceivedCallback(Callback &&...callback)
    {
        m_timer.callOnTimeout([=, this]() {
            auto frame = m_generator.generate();
            if (!frame.isValid())
                m_timer.stop();
            else
                m_timer.setInterval(m_timerInterval);
            std::invoke(callback..., frame);
        });
    }

    void run()
    {
        m_timer.start();
    }

private:
    QTimer m_timer;
    Generator m_generator;
    std::chrono::milliseconds m_timerInterval;
};

#endif // PUSHMODEMEDIASOURCE_H

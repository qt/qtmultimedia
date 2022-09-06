// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FREQUENCYMONITOR_H
#define FREQUENCYMONITOR_H

#include <QObject>
#include <QTimer>

class FrequencyMonitorPrivate;

/**
 * Class for measuring frequency of events
 *
 * Occurrence of the event is notified by the client via the notify() slot.
 * On a regular interval, both an instantaneous and a rolling average
 * event frequency are calculated.
 */
class FrequencyMonitor : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(FrequencyMonitor)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(int samplingInterval READ samplingInterval WRITE setSamplingInterval NOTIFY
                       samplingIntervalChanged)
    Q_PROPERTY(
            int traceInterval READ traceInterval WRITE setTraceInterval NOTIFY traceIntervalChanged)
    Q_PROPERTY(qreal instantaneousFrequency READ instantaneousFrequency NOTIFY
                       instantaneousFrequencyChanged)
    Q_PROPERTY(qreal averageFrequency READ averageFrequency NOTIFY averageFrequencyChanged)

public:
    FrequencyMonitor(QObject *parent = nullptr);
    ~FrequencyMonitor();

    static void qmlRegisterType();

    QString label() const;
    bool active() const;
    int samplingInterval() const;
    int traceInterval() const;
    qreal instantaneousFrequency() const;
    qreal averageFrequency() const;

signals:
    void labelChanged(const QString &value);
    void activeChanged(bool);
    void samplingIntervalChanged(int value);
    void traceIntervalChanged(int value);
    void frequencyChanged();
    void instantaneousFrequencyChanged(qreal value);
    void averageFrequencyChanged(qreal value);

public slots:
    Q_INVOKABLE void notify();
    Q_INVOKABLE void trace();
    void setActive(bool value);
    void setLabel(const QString &value);
    void setSamplingInterval(int value);
    void setTraceInterval(int value);

private:
    FrequencyMonitorPrivate *d_ptr;
};

#endif // FREQUENCYMONITOR_H

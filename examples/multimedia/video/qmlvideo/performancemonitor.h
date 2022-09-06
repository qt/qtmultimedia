// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <QByteArray>

namespace PerformanceMonitor {

struct State
{
    State() : valid(true), logging(false), visible(true) { }
    State(bool l, bool v) : valid(true), logging(l), visible(v) { }
    bool operator==(const State &other) const
    {
        return logging == other.logging && visible == other.visible;
    }
    bool operator!=(const State &other) const
    {
        return logging != other.logging || visible != other.visible;
    }

    bool parseArgument(const QByteArray &arg);

    bool valid;
    bool logging;
    bool visible;
};

} // namespace PerformanceMonitor

#endif // PERFORMANCEMONITOR_H

// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSEQUENTIALFILEADAPTOR_P
#define QSEQUENTIALFILEADAPTOR_P

#include <QtCore/qfile.h>

struct QSequentialFileAdaptor : QIODevice
{
    template <typename... Args>
    explicit QSequentialFileAdaptor(Args... args) : m_file(args...)
    {
    }

    bool open(QIODeviceBase::OpenMode mode) override
    {
        QIODevice::open(mode);
        return m_file.open(mode);
    }

    void close() override
    {
        m_file.close();
        QIODevice::close();
    }

    qint64 pos() const override { return m_file.pos(); }
    qint64 size() const override { return m_file.size(); }
    bool seek(qint64 pos) override { return m_file.seek(pos); }
    bool atEnd() const override { return m_file.atEnd(); }
    bool reset() override { return m_file.reset(); }

    qint64 bytesAvailable() const override { return m_file.bytesAvailable(); }

    bool isSequential() const override { return true; }

    qint64 readData(char *data, qint64 maxlen) override { return m_file.read(data, maxlen); }
    qint64 writeData(const char *data, qint64 len) override { return m_file.write(data, len); }

    QFile m_file;
};

#endif // QSEQUENTIALFILEADAPTOR_P

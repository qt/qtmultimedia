// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SERVER_H
#define SERVER_H

#include <private/qglobal_p.h>

#ifdef QT_FEATURE_network

#include <qstring.h>
#include <qtcpserver.h>
#include <qtest.h>
#include <qurl.h>

QT_USE_NAMESPACE

class UnResponsiveRtspServer : public QObject
{
    Q_OBJECT
public:
    UnResponsiveRtspServer() : m_server{ new QTcpServer{ this } }
    {
        connect(m_server, &QTcpServer::newConnection, this, [&] { m_connected = true; });
    }

    bool listen() { return m_server->listen(QHostAddress::LocalHost); }

    bool waitForConnection()
    {
        return QTest::qWaitFor([this] { return m_connected; });
    }

    QUrl address() const
    {
        return QUrl{ QString{ "rtsp://%1:%2" }
                             .arg(m_server->serverAddress().toString())
                             .arg(m_server->serverPort()) };
    }

private:
    QTcpServer *m_server;
    bool m_connected = false;
};

#endif // QT_FEATURE_network

#endif // SERVER_H

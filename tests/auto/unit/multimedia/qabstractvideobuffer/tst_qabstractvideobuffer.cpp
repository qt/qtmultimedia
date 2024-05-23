// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <private/qabstractvideobuffer_p.h>

class tst_QAbstractVideoBuffer : public QObject
{
    Q_OBJECT
public:
    tst_QAbstractVideoBuffer();
    ~tst_QAbstractVideoBuffer() override;

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void handleType_data();
    void handleType();
    void handle();
    void mapModeDebug_data();
    void mapModeDebug();
};

class QtTestVideoBuffer : public QHwVideoBuffer
{
public:
    QtTestVideoBuffer(QVideoFrame::HandleType type) : QHwVideoBuffer(type) { }

    MapData map(QtVideo::MapMode) override { return {}; }
    void unmap() override {}
};

tst_QAbstractVideoBuffer::tst_QAbstractVideoBuffer()
{
}

tst_QAbstractVideoBuffer::~tst_QAbstractVideoBuffer()
{
}

void tst_QAbstractVideoBuffer::initTestCase()
{
}

void tst_QAbstractVideoBuffer::cleanupTestCase()
{
}

void tst_QAbstractVideoBuffer::init()
{
}

void tst_QAbstractVideoBuffer::cleanup()
{
}

void tst_QAbstractVideoBuffer::handleType_data()
{
    QTest::addColumn<QVideoFrame::HandleType>("type");
    QTest::addColumn<QString>("stringized");

    QTest::newRow("NoHandle") << QVideoFrame::NoHandle << QStringLiteral("NoHandle");
    QTest::newRow("RhiTextureHandle") << QVideoFrame::RhiTextureHandle << QStringLiteral("RhiTextureHandle");
}

void tst_QAbstractVideoBuffer::handleType()
{
    QFETCH(QVideoFrame::HandleType, type);
    QFETCH(QString, stringized);

    QtTestVideoBuffer buffer(type);

    QCOMPARE(buffer.handleType(), type);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << type;
}

void tst_QAbstractVideoBuffer::handle()
{
    QtTestVideoBuffer buffer(QVideoFrame::NoHandle);

    QVERIFY(buffer.textureHandle(nullptr, 0) == 0);
}

void tst_QAbstractVideoBuffer::mapModeDebug_data()
{
    QTest::addColumn<QtVideo::MapMode>("mapMode");
    QTest::addColumn<QString>("stringized");

    QTest::newRow("NotMapped") << QtVideo::MapMode::NotMapped << QStringLiteral("NotMapped");
    QTest::newRow("ReadOnly") << QtVideo::MapMode::ReadOnly << QStringLiteral("ReadOnly");
    QTest::newRow("WriteOnly") << QtVideo::MapMode::WriteOnly << QStringLiteral("WriteOnly");
    QTest::newRow("ReadWrite") << QtVideo::MapMode::ReadWrite << QStringLiteral("ReadWrite");
}

void tst_QAbstractVideoBuffer::mapModeDebug()
{
    QFETCH(QtVideo::MapMode, mapMode);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << mapMode;
}

QTEST_MAIN(tst_QAbstractVideoBuffer)

#include "tst_qabstractvideobuffer.moc"

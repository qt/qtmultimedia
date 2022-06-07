// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>

#include <private/qabstractvideobuffer_p.h>

// Adds an enum, and the stringized version
#define ADD_ENUM_TEST(x) \
    QTest::newRow(#x) \
        << QVideoFrame::x \
    << QString(QLatin1String(#x));

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
    void mapMode();
    void mapModeDebug_data();
    void mapModeDebug();
};

class QtTestVideoBuffer : public QAbstractVideoBuffer
{
public:
    QtTestVideoBuffer(QVideoFrame::HandleType type) : QAbstractVideoBuffer(type) {}

    [[nodiscard]] QVideoFrame::MapMode mapMode() const override { return QVideoFrame::ReadWrite; }

    MapData map(QVideoFrame::MapMode) override { return {}; }
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

    ADD_ENUM_TEST(NoHandle);
    ADD_ENUM_TEST(RhiTextureHandle);
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

    QVERIFY(buffer.textureHandle(0) == 0);
}

void tst_QAbstractVideoBuffer::mapMode()
{
    QtTestVideoBuffer maptest(QVideoFrame::NoHandle);
    QVERIFY2(maptest.mapMode() == QVideoFrame::ReadWrite, "ReadWrite Failed");
}

void tst_QAbstractVideoBuffer::mapModeDebug_data()
{
    QTest::addColumn<QVideoFrame::MapMode>("mapMode");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(NotMapped);
    ADD_ENUM_TEST(ReadOnly);
    ADD_ENUM_TEST(WriteOnly);
    ADD_ENUM_TEST(ReadWrite);
}

void tst_QAbstractVideoBuffer::mapModeDebug()
{
    QFETCH(QVideoFrame::MapMode, mapMode);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << mapMode;
}

QTEST_MAIN(tst_QAbstractVideoBuffer)

#include "tst_qabstractvideobuffer.moc"

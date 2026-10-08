// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <rhi/qrhi.h>
#include <private/qhwvideobuffer_p.h>

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
    void mapModeDebug();
};

class QtTestVideoBuffer : public QHwVideoBuffer
{
public:
    QtTestVideoBuffer(QVideoFrame::HandleType type) : QHwVideoBuffer(type) { }

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

    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Null, {}));

    QVERIFY(buffer.textureHandle(*rhi, 0) == 0);
}

void tst_QAbstractVideoBuffer::mapModeDebug()
{
    const QMetaEnum meta = QMetaEnum::fromType<QVideoFrame::MapMode>();
    QVERIFY(meta.isValid());
}

QTEST_MAIN(tst_QAbstractVideoBuffer)

#include "tst_qabstractvideobuffer.moc"

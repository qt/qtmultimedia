// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimediaTestLib/private/qmessagespy_p.h>
#include <QtTest/qtest.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qtimer.h>

using namespace std::chrono_literals;
using namespace QtMultimediaPrivate;

Q_LOGGING_CATEGORY(testCategory, "test.messagespy")

class tst_QMessageSpy : public QObject
{
    Q_OBJECT
private slots:
    void expectMessage_succeeds();
    void expectMessage_timesOut();
    void expectMessageWithRegex_succeeds();
    void multipleExpects_allMatch();
    void multipleMessages_oneMatch();
    void unmatchedMessageForwardedToPreviousHandler();
    void unmatchedMessageForwardedToTestlib();
    void matchedMessageNotForwardedToPreviousHandler();
    void expectAfterInstall_beforeTrigger();
    void loggingCategoryEnabler_enablesDisabledCategory();
    void loggingCategoryEnabler_keepsEnabledCategory();
    void loggingCategoryEnabler_restoresOnDestruction();
};

void tst_QMessageSpy::expectMessage_succeeds()
{
    QMessageSpy spy;

    auto token = spy.expect(QtDebugMsg, "hello world");
    QTimer::singleShot(50ms, this, []() {
        qDebug("hello world");
    });

    QVERIFY(token.wait(2000ms));
    QVERIFY(token.matched());
}

void tst_QMessageSpy::expectMessage_timesOut()
{
    QMessageSpy spy;
    auto token = spy.expect(QtDebugMsg, "nonexistent message");
    QVERIFY(!token.wait(100ms));
    QVERIFY(!token.matched());
}

void tst_QMessageSpy::expectMessageWithRegex_succeeds()
{
    QMessageSpy spy;

    auto token = spy.expect(QtDebugMsg, QRegularExpression("loading file: .*\\.wav"));
    QTimer::singleShot(50ms, this, []() {
        qDebug("loading file: test.wav");
    });

    QVERIFY(token.wait(2000ms));
}

void tst_QMessageSpy::multipleExpects_allMatch()
{
    QMessageSpy spy;

    auto t1 = spy.expect(QtDebugMsg, "msg one");
    auto t2 = spy.expect(QtWarningMsg, "msg two");
    auto t3 = spy.expect(QtInfoMsg, QRegularExpression("msg thr.*"));

    QTimer::singleShot(50ms, this, []() {
        qDebug("msg one");
        qWarning("msg two");
        qInfo("msg three");
    });

    QVERIFY(t1.wait(2000ms));
    QVERIFY(t2.wait(2000ms));
    QVERIFY(t3.wait(2000ms));
}

void tst_QMessageSpy::multipleMessages_oneMatch()
{
    QMessageSpy spy;
    auto t1 = spy.expect(QtDebugMsg, "message");
    QTest::ignoreMessage(QtDebugMsg, "message");
    qDebug("message");
    qDebug("message");
    QVERIFY(t1.matched());
}

void tst_QMessageSpy::unmatchedMessageForwardedToPreviousHandler()
{
    // Install a counting handler before the spy
    static int prevCount = 0;
    prevCount = 0;
    auto prev = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &) {
        ++prevCount;
    });

    {
        QMessageSpy spy;
        auto token = spy.expect(QtDebugMsg, "expected");

        // This should NOT be consumed by the spy → forwarded to prev handler
        qWarning("unrelated warning");

        // This IS consumed
        qDebug("expected");
        QVERIFY(token.wait(1000ms));
    }

    qInstallMessageHandler(prev);

    QCOMPARE(prevCount, 1); // only the unrelated warning
}

void tst_QMessageSpy::unmatchedMessageForwardedToTestlib()
{
    QMessageSpy spy;
    auto token = spy.expect(QtDebugMsg, "expected");
    QTest::ignoreMessage(QtWarningMsg, "unrelated warning");
    qWarning("unrelated warning");
    qDebug("expected");
    QVERIFY(token.wait(1000ms));
}

void tst_QMessageSpy::matchedMessageNotForwardedToPreviousHandler()
{
    static int prevCount = 0;
    prevCount = 0;
    auto prev = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &) {
        ++prevCount;
    });

    {
        QMessageSpy spy;
        auto token = spy.expect(QtDebugMsg, "consumed message");
        qDebug("consumed message");
        QVERIFY(token.wait(1000ms));
    }

    qInstallMessageHandler(prev);

    QCOMPARE(prevCount, 0); // matched message must NOT reach previous handler
}

void tst_QMessageSpy::expectAfterInstall_beforeTrigger()
{
    // Confirm the canonical usage: expect() called before message is generated
    QMessageSpy spy;

    auto token = spy.expect(QtDebugMsg, "ordered message");
    // message generated AFTER expect()
    qDebug("ordered message");

    QVERIFY(token.wait(1000ms));
}

// QLoggingCategory disabled by default (no rule enables it) — enabler should activate it
Q_LOGGING_CATEGORY(disabledCategory, "test.messagespy.disabled")

void tst_QMessageSpy::loggingCategoryEnabler_enablesDisabledCategory()
{
    auto &cat = const_cast<QLoggingCategory &>(disabledCategory());
    const bool original = cat.isEnabled(QtDebugMsg);
    Q_ASSERT(original);
    cat.setEnabled(QtDebugMsg, false);

    {
        QLoggingCategoryEnabler enabler(disabledCategory());
        QVERIFY(disabledCategory().isEnabled(QtDebugMsg));
        QVERIFY(disabledCategory().isEnabled(QtWarningMsg));
        QVERIFY(disabledCategory().isEnabled(QtInfoMsg));
    }

    QVERIFY(!disabledCategory().isEnabled(QtDebugMsg));
    cat.setEnabled(QtDebugMsg, original);
}

void tst_QMessageSpy::loggingCategoryEnabler_keepsEnabledCategory()
{
    {
        QLoggingCategoryEnabler enabler(testCategory());
        QVERIFY(testCategory().isEnabled(QtDebugMsg));
    }

    // Still enabled — enabler must not disable what was already on
    QVERIFY(testCategory().isEnabled(QtDebugMsg));
}

void tst_QMessageSpy::loggingCategoryEnabler_restoresOnDestruction()
{
    auto &cat = const_cast<QLoggingCategory &>(disabledCategory());
    const bool original = cat.isEnabled(QtDebugMsg);
    Q_ASSERT(original);
    cat.setEnabled(QtDebugMsg, false);

    {
        QLoggingCategoryEnabler enabler(disabledCategory());
        QVERIFY(disabledCategory().isEnabled(QtDebugMsg));

        QMessageSpy spy;
        auto token = spy.expect(QtDebugMsg, "enabled message");
        qCDebug(disabledCategory) << "enabled message";
        QVERIFY(token.wait(1000ms));
    }

    QVERIFY(!disabledCategory().isEnabled(QtDebugMsg));
    cat.setEnabled(QtDebugMsg, original);
}

QTEST_MAIN(tst_QMessageSpy)
#include "tst_qmessagespy.moc"

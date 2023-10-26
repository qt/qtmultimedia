// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QSignalSpy>
#include <private/qerrorinfo_p.h>

QT_USE_NAMESPACE

enum class TestError { ErrorA, ErrorB, NoError };

using TestErrorInfo = QErrorInfo<TestError>;

class TestNotifier : public QObject
{
    Q_OBJECT
public:
signals:
    void errorOccurred(TestError error, QString errorDescription);
    void errorChanged();
};

class tst_QErrorInfo : public QObject
{
    Q_OBJECT

private slots:
    void defaultConstructor_setsNoError();
    void constructor_setsPassedError();

    void setAndNotify_setsErrorAndNotifes_data();
    void setAndNotify_setsErrorAndNotifes();
};

void tst_QErrorInfo::defaultConstructor_setsNoError()
{
    TestErrorInfo errorInfo;
    QCOMPARE(errorInfo.code(), TestError::NoError);
    QCOMPARE(errorInfo.description(), QString());
}

void tst_QErrorInfo::constructor_setsPassedError()
{
    TestErrorInfo errorInfo(TestError::ErrorB, "test error");
    QCOMPARE(errorInfo.code(), TestError::ErrorB);
    QCOMPARE(errorInfo.description(), "test error");
}

void tst_QErrorInfo::setAndNotify_setsErrorAndNotifes_data()
{
    QTest::addColumn<TestError>("initialError");
    QTest::addColumn<QString>("initialErrorDescription");
    QTest::addColumn<TestError>("error");
    QTest::addColumn<QString>("errorDescription");
    QTest::addColumn<bool>("errorChangedEmitted");
    QTest::addColumn<bool>("errorOccurredEmitted");

    QTest::newRow("No error -> No error")
            << TestError::NoError << QString() << TestError::NoError << QString() << false << false;
    QTest::newRow("No error -> An error with empty string")
            << TestError::NoError << QString() << TestError::ErrorA << QString() << true << true;
    QTest::newRow("No error -> An error with non-empty string")
            << TestError::NoError << QString() << TestError::ErrorB << QString("error") << true
            << true;
    QTest::newRow("An error with empty string -> No error")
            << TestError::ErrorA << QString() << TestError::NoError << QString() << true << false;
    QTest::newRow("An error with non-empty string -> No Error")
            << TestError::ErrorA << QString("error") << TestError::NoError << QString() << true
            << false;
    QTest::newRow("An error -> Another error")
            << TestError::ErrorA << QString("error A") << TestError::ErrorB << QString("error B")
            << true << true;
    QTest::newRow("An error -> Another error with empty string")
            << TestError::ErrorA << QString("error A") << TestError::ErrorB << QString() << true
            << true;
    QTest::newRow("An error -> The same error with changed string")
            << TestError::ErrorA << QString("error") << TestError::ErrorA
            << QString("another error") << true << true;
}

void tst_QErrorInfo::setAndNotify_setsErrorAndNotifes()
{
    QFETCH(TestError, initialError);
    QFETCH(QString, initialErrorDescription);
    QFETCH(TestError, error);
    QFETCH(QString, errorDescription);
    QFETCH(bool, errorChangedEmitted);
    QFETCH(bool, errorOccurredEmitted);

    TestErrorInfo errorInfo(initialError, initialErrorDescription);

    TestNotifier notifier;
    QSignalSpy errorOccurredSpy(&notifier, &TestNotifier::errorOccurred);
    QSignalSpy errorChangedSpy(&notifier, &TestNotifier::errorChanged);

    errorInfo.setAndNotify(error, errorDescription, notifier);

    QCOMPARE(errorInfo.code(), error);
    QCOMPARE(errorInfo.description(), errorDescription);

    QList<QList<QVariant>> expectedErrorChanged;
    if (errorChangedEmitted)
        expectedErrorChanged.push_back({});

    QList<QList<QVariant>> expectedErrorOccured;
    if (errorOccurredEmitted)
        expectedErrorOccured.push_back({ QVariant::fromValue(error), errorDescription });

    QCOMPARE(errorOccurredSpy, expectedErrorOccured);
    QCOMPARE(errorChangedSpy, expectedErrorChanged);
}

QTEST_GUILESS_MAIN(tst_QErrorInfo)

#include "tst_qerrorinfo.moc"

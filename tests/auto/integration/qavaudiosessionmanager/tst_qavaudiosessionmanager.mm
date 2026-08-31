// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtMultimedia/private/qavaudiosessionmanager_p.h>

#include <optional>

#import <AVFoundation/AVAudioSession.h>
#import <Foundation/Foundation.h>

using QtMultimediaPrivate::QAVAudioSessionManager;

namespace {

bool categoryEquals(AVAudioSessionCategory lhs, AVAudioSessionCategory rhs)
{
    return [lhs isEqualToString:rhs];
}

// Posts a synthetic AVAudioSessionInterruptionNotification. QAVAudioSessionManager observes
// object:[AVAudioSession sharedInstance], so a notification posted this way from within the test
// process reaches the exact same handler chain a real interruption would.
void postInterruptionNotification(
        AVAudioSessionInterruptionType type,
        std::optional<AVAudioSessionInterruptionOptions> options = std::nullopt)
{
    NSMutableDictionary<NSString *, id> *userInfo = [NSMutableDictionary dictionary];
    userInfo[AVAudioSessionInterruptionTypeKey] = @(type);
    if (options)
        userInfo[AVAudioSessionInterruptionOptionKey] = @(*options);

    [NSNotificationCenter.defaultCenter postNotificationName:AVAudioSessionInterruptionNotification
                                                      object:AVAudioSession.sharedInstance
                                                    userInfo:userInfo];
}

void postMediaServicesNotification(NSNotificationName name)
{
    [NSNotificationCenter.defaultCenter postNotificationName:name
                                                      object:AVAudioSession.sharedInstance
                                                    userInfo:nil];
}

// Sets the shared AVAudioSession's category directly, bypassing QAVAudioSessionManager, so tests
// can simulate a category drift that the manager was never told about via updateConfiguration().
void applyCategoryDirectly(AVAudioSessionCategory category)
{
    NSError *error = nil;
    const BOOL success = [AVAudioSession.sharedInstance setCategory:category error:&error];
    QVERIFY2(success, qPrintable(QString::fromNSString(error.localizedDescription)));
}

} // namespace

class tst_QAVAudioSessionManager : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void instanceIsStableSingleton();

    void interruptionBeganEmitsSignal();
    void interruptionEndedEmitsSignal_data();
    void interruptionEndedEmitsSignal();
    void mediaServicesWereLostEmitsSignal();
    void mediaServicesWereResetEmitsSignal();

    void activateSessionRestoresDriftedConfiguration();
    void activateSessionDoesNotClobberUpdatedConfiguration();
    void activateSessionIsNoopWhenConfigurationUnchanged();

private:
    AVAudioSessionCategory m_initialCategory;
};

void tst_QAVAudioSessionManager::init()
{
    m_initialCategory = AVAudioSession.sharedInstance.category;
}

void tst_QAVAudioSessionManager::cleanup()
{
    applyCategoryDirectly(m_initialCategory);
    QAVAudioSessionManager::instance()->updateConfiguration();
}

void tst_QAVAudioSessionManager::instanceIsStableSingleton()
{
    QAVAudioSessionManager *first = QAVAudioSessionManager::instance();
    QAVAudioSessionManager *second = QAVAudioSessionManager::instance();

    QVERIFY(first);
    QCOMPARE(first, second);
}

void tst_QAVAudioSessionManager::interruptionBeganEmitsSignal()
{
    QSignalSpy spy(QAVAudioSessionManager::instance(), &QAVAudioSessionManager::interruptionBegan);

    postInterruptionNotification(AVAudioSessionInterruptionTypeBegan);

    QTRY_COMPARE_EQ(spy.size(), 1);
}

void tst_QAVAudioSessionManager::interruptionEndedEmitsSignal_data()
{
    QTest::addColumn<bool>("optionPresent");
    QTest::addColumn<bool>("expectedShouldResume");

    // AVAudioSessionInterruptionOptionKey may be absent on "ended"; absence means "do not resume".
    QTest::newRow("ShouldResume option present") << true << true;
    QTest::newRow("no option key at all") << false << false;
}

void tst_QAVAudioSessionManager::interruptionEndedEmitsSignal()
{
    QFETCH(bool, optionPresent);
    QFETCH(bool, expectedShouldResume);

    QSignalSpy spy(QAVAudioSessionManager::instance(), &QAVAudioSessionManager::interruptionEnded);

    if (optionPresent)
        postInterruptionNotification(AVAudioSessionInterruptionTypeEnded,
                                     AVAudioSessionInterruptionOptionShouldResume);
    else
        postInterruptionNotification(AVAudioSessionInterruptionTypeEnded);

    QTRY_COMPARE_EQ(spy.size(), 1);
    QCOMPARE(spy.constFirst().constFirst().toBool(), expectedShouldResume);
}

void tst_QAVAudioSessionManager::mediaServicesWereLostEmitsSignal()
{
    QSignalSpy spy(QAVAudioSessionManager::instance(),
                   &QAVAudioSessionManager::mediaServicesWereLost);

    postMediaServicesNotification(AVAudioSessionMediaServicesWereLostNotification);

    QTRY_COMPARE_EQ(spy.size(), 1);
}

void tst_QAVAudioSessionManager::mediaServicesWereResetEmitsSignal()
{
    QSignalSpy spy(QAVAudioSessionManager::instance(),
                   &QAVAudioSessionManager::mediaServicesWereReset);

    postMediaServicesNotification(AVAudioSessionMediaServicesWereResetNotification);

    QTRY_COMPARE_EQ(spy.size(), 1);
}

void tst_QAVAudioSessionManager::activateSessionRestoresDriftedConfiguration()
{
    applyCategoryDirectly(AVAudioSessionCategoryAmbient);
    QAVAudioSessionManager::instance()->updateConfiguration();

    // Simulate a category drift the manager was never told about.
    applyCategoryDirectly(AVAudioSessionCategoryPlayback);

    QVERIFY(QAVAudioSessionManager::instance()->activateSession());
    QVERIFY(categoryEquals(AVAudioSession.sharedInstance.category, AVAudioSessionCategoryAmbient));
}

void tst_QAVAudioSessionManager::activateSessionDoesNotClobberUpdatedConfiguration()
{
    applyCategoryDirectly(AVAudioSessionCategoryAmbient);
    QAVAudioSessionManager::instance()->updateConfiguration();

    // Unlike activateSessionRestoresDriftedConfiguration(), this later change is reported via
    // updateConfiguration(), the same way avfmediaplayer.mm reports its own category changes.
    applyCategoryDirectly(AVAudioSessionCategoryPlayback);
    QAVAudioSessionManager::instance()->updateConfiguration();

    QVERIFY(QAVAudioSessionManager::instance()->activateSession());
    QVERIFY(categoryEquals(AVAudioSession.sharedInstance.category, AVAudioSessionCategoryPlayback));
}

void tst_QAVAudioSessionManager::activateSessionIsNoopWhenConfigurationUnchanged()
{
    applyCategoryDirectly(AVAudioSessionCategoryAmbient);
    QAVAudioSessionManager::instance()->updateConfiguration();

    QVERIFY(QAVAudioSessionManager::instance()->activateSession());
    QVERIFY(categoryEquals(AVAudioSession.sharedInstance.category, AVAudioSessionCategoryAmbient));
}

QTEST_MAIN(tst_QAVAudioSessionManager)

#include "tst_qavaudiosessionmanager.moc"

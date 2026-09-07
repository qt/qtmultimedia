// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtMultimedia/private/qavaudiosessionmanager_p.h>

#include <optional>

#import <AVFoundation/AVAudioSession.h>
#import <Foundation/Foundation.h>

using QtMultimediaPrivate::QAVAudioSessionManager;
using QtMultimediaPrivate::SessionRequirement;
using ActivationToken = QAVAudioSessionManager::ActivationToken;

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
// can simulate a drift the manager was never told about (e.g. what the OS leaves behind after an
// interruption or media services reset).
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

    void activateAppliesPlaybackCategoryByDefault();
    void activateAppliesPlayAndRecordCategoryWhenRecordingRequested();
    void activateUnionsOptionsAcrossConcurrentTokens();
    void releasingTokenRecomputesMergedCategory();
    void moveAssigningTokenReleasesPreviousRequirement();
    void reassertActivationRestoresCategoryFromLiveTokens();
    void reassertActivationWithNoLiveTokensJustActivates();

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

// setActive:NO on last-token-release has no public AVAudioSession getter to observe directly.
// It's exercised indirectly by every test below starting from a deactivated session (all tokens
// from the previous test having gone out of scope) and successfully reactivating.

void tst_QAVAudioSessionManager::activateAppliesPlaybackCategoryByDefault()
{
    ActivationToken token = QAVAudioSessionManager::instance()->activate(
            { /* needsRecord = */ false, AVAudioSessionCategoryOptionMixWithOthers });

    QVERIFY(categoryEquals(AVAudioSession.sharedInstance.category, AVAudioSessionCategoryPlayback));
    QVERIFY(AVAudioSession.sharedInstance.categoryOptions
            & AVAudioSessionCategoryOptionMixWithOthers);
}

void tst_QAVAudioSessionManager::activateAppliesPlayAndRecordCategoryWhenRecordingRequested()
{
    ActivationToken token =
            QAVAudioSessionManager::instance()->activate({ /* needsRecord = */ true });

    QVERIFY(categoryEquals(AVAudioSession.sharedInstance.category,
                           AVAudioSessionCategoryPlayAndRecord));
}

void tst_QAVAudioSessionManager::activateUnionsOptionsAcrossConcurrentTokens()
{
    ActivationToken mixToken = QAVAudioSessionManager::instance()->activate(
            { /* needsRecord = */ false, AVAudioSessionCategoryOptionMixWithOthers });
    ActivationToken duckToken = QAVAudioSessionManager::instance()->activate(
            { /* needsRecord = */ false, AVAudioSessionCategoryOptionDuckOthers });

    const AVAudioSessionCategoryOptions options = AVAudioSession.sharedInstance.categoryOptions;
    QVERIFY(options & AVAudioSessionCategoryOptionMixWithOthers);
    QVERIFY(options & AVAudioSessionCategoryOptionDuckOthers);
}

void tst_QAVAudioSessionManager::releasingTokenRecomputesMergedCategory()
{
    ActivationToken playbackToken =
            QAVAudioSessionManager::instance()->activate({ /* needsRecord = */ false });

    {
        ActivationToken recordToken =
                QAVAudioSessionManager::instance()->activate({ /* needsRecord = */ true });
        QVERIFY(categoryEquals(AVAudioSession.sharedInstance.category,
                               AVAudioSessionCategoryPlayAndRecord));
    }

    QVERIFY(categoryEquals(AVAudioSession.sharedInstance.category, AVAudioSessionCategoryPlayback));
}

void tst_QAVAudioSessionManager::moveAssigningTokenReleasesPreviousRequirement()
{
    ActivationToken token =
            QAVAudioSessionManager::instance()->activate({ /* needsRecord = */ true });
    QVERIFY(categoryEquals(AVAudioSession.sharedInstance.category,
                           AVAudioSessionCategoryPlayAndRecord));

    // Move-assigning a newly-activated token over `token` must release the first requirement
    // before installing the second, otherwise the category would stay merged as PlayAndRecord.
    token = QAVAudioSessionManager::instance()->activate({ /* needsRecord = */ false });

    QVERIFY(categoryEquals(AVAudioSession.sharedInstance.category, AVAudioSessionCategoryPlayback));
}

void tst_QAVAudioSessionManager::reassertActivationRestoresCategoryFromLiveTokens()
{
    ActivationToken token =
            QAVAudioSessionManager::instance()->activate({ /* needsRecord = */ true });

    // Simulate what the OS leaves behind after an interruption/media services reset.
    applyCategoryDirectly(AVAudioSessionCategoryAmbient);

    QVERIFY(QAVAudioSessionManager::instance()->reassertActivation());
    QVERIFY(categoryEquals(AVAudioSession.sharedInstance.category,
                           AVAudioSessionCategoryPlayAndRecord));
}

void tst_QAVAudioSessionManager::reassertActivationWithNoLiveTokensJustActivates()
{
    const AVAudioSessionCategory categoryBefore = AVAudioSession.sharedInstance.category;

    QVERIFY(QAVAudioSessionManager::instance()->reassertActivation());

    QVERIFY(categoryEquals(AVAudioSession.sharedInstance.category, categoryBefore));
}

QTEST_MAIN(tst_QAVAudioSessionManager)

#include "tst_qavaudiosessionmanager.moc"

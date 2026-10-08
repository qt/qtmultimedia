// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// TESTED_COMPONENT=src/multimedia

#include <QtTest/qtest.h>
#include <private/qvideotransformation_p.h>
#include <map>

QT_USE_NAMESPACE

using namespace QtVideo;

struct VideoTransformationLess
{
    bool operator()(const VideoTransformation &lhs, const VideoTransformation &rhs) const
    {
        return std::tie(lhs.rotation, lhs.mirroredHorizontallyAfterRotation)
                < std::tie(rhs.rotation, rhs.mirroredHorizontallyAfterRotation);
    }
};

using VideoTransformationMap =
        std::map<VideoTransformation, VideoTransformation, VideoTransformationLess>;

class tst_VideoTransformation : public QObject
{
    Q_OBJECT

private slots:
    void defaultConstructor_setsNoTransform();

    void rotationIndex_returnsIndexAccordingToRotation_data();
    void rotationIndex_returnsIndexAccordingToRotation();

    void equalityOperators_compareAllMembers();

    void mirrorHorizontally_data();
    void mirrorHorizontally();

    void mirrorVertically_data();
    void mirrorVertically();

    void rotate_none_data();
    void rotate_none();

    void rotate_90_data();
    void rotate_90();

    void rotate_180_data();
    void rotate_180();

    void rotate_270_data();
    void rotate_270();

private:
    void generateAllTransformationsData(const VideoTransformationMap &expectedMap = {});

    template <typename Transformer>
    void doTransformTest(Transformer &&transformer)
    {
        QFETCH(const VideoTransformation, transform);
        QFETCH(const VideoTransformation, expected);

        VideoTransformation actual = transform;
        transformer(actual);

        QCOMPARE(actual, expected);
    }
};

void tst_VideoTransformation::generateAllTransformationsData(
        const VideoTransformationMap &expectedMap)
{
    QTest::addColumn<VideoTransformation>("transform");
    QTest::addColumn<int>("rotationIndex");
    QTest::addColumn<VideoTransformation>("expected");

    for (int rotation = 0; rotation < 360; rotation += 90)
        for (bool mirorred : { false, true }) {
            const QString tag =
                    QStringLiteral("rotation %1; mirrored %2")
                            .arg(QString::number(rotation), mirorred ? u"true" : u"false");
            const VideoTransformation transform{ Rotation(rotation), mirorred };

            QTestData &row = QTest::newRow(tag.toLatin1().constData())
                    << transform << rotation / 90;

            const auto it = expectedMap.find(transform);
            row << (it == expectedMap.end() ? transform : it->second);
        }
}

void tst_VideoTransformation::defaultConstructor_setsNoTransform()
{
    VideoTransformation transform;
    QCOMPARE(transform.rotation, QtVideo::Rotation::None);
    QVERIFY(!transform.mirroredHorizontallyAfterRotation);
}

void tst_VideoTransformation::rotationIndex_returnsIndexAccordingToRotation_data()
{
    generateAllTransformationsData();
}

void tst_VideoTransformation::rotationIndex_returnsIndexAccordingToRotation()
{
    QFETCH(const VideoTransformation, transform);
    QFETCH(const int, rotationIndex);

    QCOMPARE(transform.rotationIndex(), rotationIndex);
}

void tst_VideoTransformation::equalityOperators_compareAllMembers()
{
    QVERIFY(VideoTransformation{} == VideoTransformation{});
    QVERIFY((VideoTransformation{ Rotation::Clockwise90, true }
             == VideoTransformation{ Rotation::Clockwise90, true }));

    QVERIFY(!(VideoTransformation{} != VideoTransformation{}));
    QVERIFY(!(VideoTransformation{ Rotation::Clockwise180, true }
              != VideoTransformation{ Rotation::Clockwise180, true }));

    QVERIFY(!(VideoTransformation{ Rotation::Clockwise90, true }
              == VideoTransformation{ Rotation::Clockwise180, true }));
    QVERIFY(!(VideoTransformation{ Rotation::Clockwise90, true }
              == VideoTransformation{ Rotation::Clockwise90, false }));

    QVERIFY((VideoTransformation{ Rotation::Clockwise90, true }
             != VideoTransformation{ Rotation::Clockwise180, true }));
    QVERIFY((VideoTransformation{ Rotation::Clockwise90, true }
             != VideoTransformation{ Rotation::Clockwise90, false }));
}

void tst_VideoTransformation::mirrorHorizontally_data()
{
    generateAllTransformationsData({
            { {}, { Rotation::None, true } },
            { { Rotation::Clockwise90 }, { Rotation::Clockwise90, true } },
            { { Rotation::Clockwise180 }, { Rotation::Clockwise180, true } },
            { { Rotation::Clockwise270 }, { Rotation::Clockwise270, true } },
            { { Rotation::None, true }, { Rotation::None } },
            { { Rotation::Clockwise90, true }, { Rotation::Clockwise90 } },
            { { Rotation::Clockwise180, true }, { Rotation::Clockwise180 } },
            { { Rotation::Clockwise270, true }, { Rotation::Clockwise270 } },
    });
}

void tst_VideoTransformation::mirrorHorizontally()
{
    doTransformTest([](VideoTransformation &transform) { transform.mirrorHorizontally(); });
}

void tst_VideoTransformation::mirrorVertically_data()
{
    generateAllTransformationsData({
            { {}, { Rotation::Clockwise180, true } },
            { { Rotation::Clockwise90 }, { Rotation::Clockwise270, true } },
            { { Rotation::Clockwise180 }, { Rotation::None, true } },
            { { Rotation::Clockwise270 }, { Rotation::Clockwise90, true } },
            { { Rotation::None, true }, { Rotation::Clockwise180 } },
            { { Rotation::Clockwise90, true }, { Rotation::Clockwise270 } },
            { { Rotation::Clockwise180, true }, { Rotation::None } },
            { { Rotation::Clockwise270, true }, { Rotation::Clockwise90 } },
    });
}

void tst_VideoTransformation::mirrorVertically()
{
    doTransformTest([](VideoTransformation &transform) { transform.mirrorVertically(); });
}

void tst_VideoTransformation::rotate_none_data()
{
    generateAllTransformationsData();
}

void tst_VideoTransformation::rotate_none()
{
    doTransformTest([](VideoTransformation &transform) { transform.rotate(Rotation::None); });
}

void tst_VideoTransformation::rotate_90_data()
{
    generateAllTransformationsData({
            { {}, { Rotation::Clockwise90 } },
            { { Rotation::Clockwise90 }, { Rotation::Clockwise180 } },
            { { Rotation::Clockwise180 }, { Rotation::Clockwise270 } },
            { { Rotation::Clockwise270 }, { Rotation::None } },
            { { Rotation::None, true }, { Rotation::Clockwise270, true } },
            { { Rotation::Clockwise90, true }, { Rotation::None, true } },
            { { Rotation::Clockwise180, true }, { Rotation::Clockwise90, true } },
            { { Rotation::Clockwise270, true }, { Rotation::Clockwise180, true } },
    });
}

void tst_VideoTransformation::rotate_90()
{
    doTransformTest(
            [](VideoTransformation &transform) { transform.rotate(Rotation::Clockwise90); });
}

void tst_VideoTransformation::rotate_180_data()
{
    generateAllTransformationsData({
            { {}, { Rotation::Clockwise180 } },
            { { Rotation::Clockwise90 }, { Rotation::Clockwise270 } },
            { { Rotation::Clockwise180 }, { Rotation::None } },
            { { Rotation::Clockwise270 }, { Rotation::Clockwise90 } },
            { { Rotation::None, true }, { Rotation::Clockwise180, true } },
            { { Rotation::Clockwise90, true }, { Rotation::Clockwise270, true } },
            { { Rotation::Clockwise180, true }, { Rotation::None, true } },
            { { Rotation::Clockwise270, true }, { Rotation::Clockwise90, true } },
    });
}

void tst_VideoTransformation::rotate_180()
{
    doTransformTest(
            [](VideoTransformation &transform) { transform.rotate(Rotation::Clockwise180); });
}

void tst_VideoTransformation::rotate_270_data()
{
    generateAllTransformationsData({
            { {}, { Rotation::Clockwise270 } },
            { { Rotation::Clockwise90 }, { Rotation::None } },
            { { Rotation::Clockwise180 }, { Rotation::Clockwise90 } },
            { { Rotation::Clockwise270 }, { Rotation::Clockwise180 } },
            { { Rotation::None, true }, { Rotation::Clockwise90, true } },
            { { Rotation::Clockwise90, true }, { Rotation::Clockwise180, true } },
            { { Rotation::Clockwise180, true }, { Rotation::Clockwise270, true } },
            { { Rotation::Clockwise270, true }, { Rotation::None, true } },
    });
}

void tst_VideoTransformation::rotate_270()
{
    doTransformTest(
            [](VideoTransformation &transform) { transform.rotate(Rotation::Clockwise270); });
}

QTEST_GUILESS_MAIN(tst_VideoTransformation)

#include "tst_qvideotransformation.moc"

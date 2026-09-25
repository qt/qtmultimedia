// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <private/qpipewire_cameranode_p.h>

using namespace QtPipeWire;
using namespace Qt::StringLiterals;

namespace {

PwPropertyDict v4l2Camera()
{
    return {
        { "media.class", "Video/Source" },
        { "device.api", "v4l2" },
        { "node.name", "v4l2_input.pci-0000_00_14_0-usb-0_2_1_0" },
        { "node.description", "Integrated Camera" },
        { "object.serial", "42" },
    };
}

} // namespace

class tst_QPipeWireCameraNode : public QObject
{
    Q_OBJECT

private slots:
    void isCameraNode_data();
    void isCameraNode();

    void cameraDeviceId_prefersNodeName();
    void cameraDeviceId_fallsBackToObjectPath();
    void cameraDeviceId_fallsBackToObjectSerial();
    void cameraDeviceId_isEmpty_whenNothingIdentifiesTheNode();
    void cameraDeviceId_isPrefixed_soItCannotCollideWithV4l2Ids();

    void cameraDescription_data();
    void cameraDescription();

    void cameraPosition_data();
    void cameraPosition();
};

void tst_QPipeWireCameraNode::isCameraNode_data()
{
    QTest::addColumn<PwPropertyDict>("properties");
    QTest::addColumn<bool>("expected");

    QTest::newRow("v4l2 camera") << v4l2Camera() << true;

    QTest::newRow("libcamera camera") << PwPropertyDict{
        { "media.class", "Video/Source" },
        { "device.api", "libcamera" },
    } << true;

    QTest::newRow("media.role Camera, unknown device.api") << PwPropertyDict{
        { "media.class", "Video/Source" },
        { "media.role", "Camera" },
        { "device.api", "something-new" },
    } << true;

    // What the camera portal exposes: a role but no device.api at all.
    QTest::newRow("portal node") << PwPropertyDict{
        { "media.class", "Video/Source" },
        { "media.role", "Camera" },
    } << true;

    // An application producing video registers under a different media class.
    QTest::newRow("screencast stream") << PwPropertyDict{
        { "media.class", "Stream/Output/Video" },
        { "media.role", "Camera" },
    } << false;

    QTest::newRow("audio source") << PwPropertyDict{
        { "media.class", "Audio/Source" },
        { "device.api", "alsa" },
    } << false;

    // A virtual source: right media class, but neither a camera role nor a
    // capture-hardware monitor behind it.
    QTest::newRow("virtual video source") << PwPropertyDict{
        { "media.class", "Video/Source" },
        { "node.name", "obs-virtualcam" },
    } << false;

    QTest::newRow("no media.class") << PwPropertyDict{
        { "device.api", "v4l2" },
    } << false;

    QTest::newRow("empty") << PwPropertyDict{} << false;
}

void tst_QPipeWireCameraNode::isCameraNode()
{
    QFETCH(const PwPropertyDict, properties);
    QFETCH(const bool, expected);

    QCOMPARE(QtPipeWire::isCameraNode(properties), expected);
}

void tst_QPipeWireCameraNode::cameraDeviceId_prefersNodeName()
{
    QCOMPARE(cameraDeviceId(v4l2Camera()), "pw:v4l2_input.pci-0000_00_14_0-usb-0_2_1_0");
}

void tst_QPipeWireCameraNode::cameraDeviceId_fallsBackToObjectPath()
{
    const PwPropertyDict properties{
        { "media.class", "Video/Source" },
        { "object.path", "v4l2:/dev/video0" },
        { "object.serial", "42" },
    };

    QCOMPARE(cameraDeviceId(properties), "pw:v4l2:/dev/video0");
}

void tst_QPipeWireCameraNode::cameraDeviceId_fallsBackToObjectSerial()
{
    const PwPropertyDict properties{
        { "media.class", "Video/Source" },
        { "object.serial", "42" },
    };

    QCOMPARE(cameraDeviceId(properties), "pw:serial:42");
}

void tst_QPipeWireCameraNode::cameraDeviceId_isEmpty_whenNothingIdentifiesTheNode()
{
    QCOMPARE(cameraDeviceId(PwPropertyDict{}), QByteArray{});
}

void tst_QPipeWireCameraNode::cameraDeviceId_isPrefixed_soItCannotCollideWithV4l2Ids()
{
    // The V4L2 backend uses the device path verbatim as its id, so a persisted
    // id has to say which backend produced it.
    const PwPropertyDict properties{
        { "node.name", "/dev/video0" },
    };

    QCOMPARE_NE(cameraDeviceId(properties), "/dev/video0");
    QVERIFY(cameraDeviceId(properties).startsWith("pw:"));
}

void tst_QPipeWireCameraNode::cameraDescription_data()
{
    QTest::addColumn<PwPropertyDict>("nodeProperties");
    QTest::addColumn<PwPropertyDict>("deviceProperties");
    QTest::addColumn<QString>("expected");

    QTest::newRow("node.description wins")
            << PwPropertyDict{ { "node.description", "Integrated Camera" },
                               { "node.nick", "nick" },
                               { "node.name", "name" } }
            << PwPropertyDict{ { "device.description", "device" } } << u"Integrated Camera"_s;

    QTest::newRow("node.nick before device.description")
            << PwPropertyDict{ { "node.nick", "nick" }, { "node.name", "name" } }
            << PwPropertyDict{ { "device.description", "device" } } << u"nick"_s;

    QTest::newRow("device.description before node.name")
            << PwPropertyDict{ { "node.name", "name" } }
            << PwPropertyDict{ { "device.description", "device" } } << u"device"_s;

    QTest::newRow("node.name as last resort")
            << PwPropertyDict{ { "node.name", "name" } } << PwPropertyDict{} << u"name"_s;

    QTest::newRow("nothing at all") << PwPropertyDict{} << PwPropertyDict{} << QString{};

    QTest::newRow("non-ascii is decoded as utf-8")
            << PwPropertyDict{ { "node.description", "Kamera \xc3\xa4\xc3\xb6" } }
            << PwPropertyDict{} << u"Kamera äö"_s;
}

void tst_QPipeWireCameraNode::cameraDescription()
{
    QFETCH(const PwPropertyDict, nodeProperties);
    QFETCH(const PwPropertyDict, deviceProperties);
    QFETCH(const QString, expected);

    QCOMPARE(QtPipeWire::cameraDescription(nodeProperties, deviceProperties), expected);
}

void tst_QPipeWireCameraNode::cameraPosition_data()
{
    QTest::addColumn<PwPropertyDict>("properties");
    QTest::addColumn<QCameraDevice::Position>("expected");

    QTest::newRow("libcamera front") << PwPropertyDict{ { "device.api", "libcamera" },
                                                        { "api.libcamera.location", "front" } }
                                     << QCameraDevice::FrontFace;

    QTest::newRow("libcamera back")
            << PwPropertyDict{ { "device.api", "libcamera" }, { "api.libcamera.location", "back" } }
            << QCameraDevice::BackFace;

    QTest::newRow("libcamera external")
            << PwPropertyDict{ { "device.api", "libcamera" },
                               { "api.libcamera.location", "external" } }
            << QCameraDevice::UnspecifiedPosition;

    QTest::newRow("libcamera without a location") << PwPropertyDict{ { "device.api", "libcamera" } }
                                                  << QCameraDevice::UnspecifiedPosition;

    // v4l2 has no notion of where a camera faces, so a stray property must not
    // be believed.
    QTest::newRow("v4l2 ignores the location")
            << PwPropertyDict{ { "device.api", "v4l2" }, { "api.libcamera.location", "front" } }
            << QCameraDevice::UnspecifiedPosition;
}

void tst_QPipeWireCameraNode::cameraPosition()
{
    QFETCH(const PwPropertyDict, properties);
    QFETCH(const QCameraDevice::Position, expected);

    QCOMPARE(QtPipeWire::cameraPosition(properties), expected);
}

QTEST_GUILESS_MAIN(tst_QPipeWireCameraNode)

#include "tst_qpipewire_cameranode.moc"

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <private/qpipewire_cameradevicemonitor_p.h>
#include <private/qpipewire_cameranode_p.h>

#include <pipewire/extensions/metadata.h>

using namespace QtPipeWire;
using namespace Qt::StringLiterals;

namespace {

// Builds an spa_dict backed by a PwPropertyDict, so tests can write out
// properties with the same map-literal syntax the rest of the pipewire tests
// use, rather than an array of spa_dict_item.
struct DictFixture
{
    explicit DictFixture(PwPropertyDict props) : properties(std::move(props))
    {
        for (const auto &[key, value] : properties)
            items.push_back(spa_dict_item{ key.c_str(), value.c_str() });
    }

    spa_dict dict() const
    {
        return SPA_DICT_INIT(const_cast<spa_dict_item *>(items.data()), uint32_t(items.size()));
    }

    PwPropertyDict properties;
    std::vector<spa_dict_item> items;
};

DictFixture v4l2NodeProperties(const char *nodeName, const char *serial = "1")
{
    return DictFixture{ PwPropertyDict{
            { "media.class", "Video/Source" },
            { "device.api", "v4l2" },
            { "node.name", nodeName },
            { "object.serial", serial },
    } };
}

DictFixture deviceProperties(const char *description)
{
    return DictFixture{ PwPropertyDict{
            { "device.description", description },
    } };
}

} // namespace

class tst_QPipeWireCameraDeviceMonitor : public QObject
{
    Q_OBJECT

private slots:
    void objectAdded_addsCameraNode_visibleInCameraNodes();
    void objectAdded_ignoresNonCameraNode();

    void cameraNodes_isSortedByDeviceId_regardlessOfInsertionOrder();

    void findCameraNode_returnsId_whenDeviceIdMatches();
    void findCameraNode_returnsNullopt_whenNoMatch();

    void findTargetObject_prefersObjectSerial_overNodeName();
    void findTargetObject_returnsNullopt_whenNoMatch();

    void objectAdded_deviceAfterNode_nodeBorrowsDescription();
    void objectAdded_deviceBeforeNode_nodeBorrowsDescription();

    void objectRemoved_removesNode();
    void objectRemoved_deviceRemoval_clearsBorrowedDescriptionOnSurvivingNode();

    void objectAdded_emitsCamerasChanged_forCameraNode();
    void objectAdded_doesNotEmitCamerasChanged_forNonCameraNode();
};

void tst_QPipeWireCameraDeviceMonitor::objectAdded_addsCameraNode_visibleInCameraNodes()
{
    QCameraDeviceMonitor monitor;
    const DictFixture node = v4l2NodeProperties("cam0");

    monitor.objectAdded(ObjectId{ 1 }, PipewireRegistryType::Node, node.dict());

    const std::vector nodes = monitor.cameraNodes();
    QCOMPARE(nodes.size(), 1);
    QCOMPARE(nodes.front().id, ObjectId{ 1 });
    QCOMPARE(cameraDeviceId(nodes.front().properties), "pw:cam0");
}

void tst_QPipeWireCameraDeviceMonitor::objectAdded_ignoresNonCameraNode()
{
    QCameraDeviceMonitor monitor;
    const DictFixture audioNode{ PwPropertyDict{
            { "media.class", "Audio/Source" },
            { "device.api", "alsa" },
    } };

    monitor.objectAdded(ObjectId{ 1 }, PipewireRegistryType::Node, audioNode.dict());

    QVERIFY(monitor.cameraNodes().empty());
}

void tst_QPipeWireCameraDeviceMonitor::cameraNodes_isSortedByDeviceId_regardlessOfInsertionOrder()
{
    QCameraDeviceMonitor monitor;
    const DictFixture nodeB = v4l2NodeProperties("cam_b");
    const DictFixture nodeA = v4l2NodeProperties("cam_a");

    // Inserted in reverse order; the id (derived from node.name) is what
    // fixes the order, not object id or insertion order.
    monitor.objectAdded(ObjectId{ 2 }, PipewireRegistryType::Node, nodeB.dict());
    monitor.objectAdded(ObjectId{ 1 }, PipewireRegistryType::Node, nodeA.dict());

    const std::vector nodes = monitor.cameraNodes();
    QCOMPARE(nodes.size(), 2);
    QCOMPARE(cameraDeviceId(nodes[0].properties), "pw:cam_a");
    QCOMPARE(cameraDeviceId(nodes[1].properties), "pw:cam_b");
}

void tst_QPipeWireCameraDeviceMonitor::findCameraNode_returnsId_whenDeviceIdMatches()
{
    QCameraDeviceMonitor monitor;
    const DictFixture node = v4l2NodeProperties("cam0");
    monitor.objectAdded(ObjectId{ 7 }, PipewireRegistryType::Node, node.dict());

    QCOMPARE(monitor.findCameraNode("pw:cam0"), ObjectId{ 7 });
}

void tst_QPipeWireCameraDeviceMonitor::findCameraNode_returnsNullopt_whenNoMatch()
{
    QCameraDeviceMonitor monitor;

    QCOMPARE(monitor.findCameraNode("pw:does-not-exist"), std::nullopt);
}

void tst_QPipeWireCameraDeviceMonitor::findTargetObject_prefersObjectSerial_overNodeName()
{
    QCameraDeviceMonitor monitor;
    const DictFixture node = v4l2NodeProperties("cam0", "1234");
    monitor.objectAdded(ObjectId{ 1 }, PipewireRegistryType::Node, node.dict());

    QCOMPARE(monitor.findTargetObject("pw:cam0"), "1234");
}

void tst_QPipeWireCameraDeviceMonitor::findTargetObject_returnsNullopt_whenNoMatch()
{
    QCameraDeviceMonitor monitor;

    QCOMPARE(monitor.findTargetObject("pw:does-not-exist"), std::nullopt);
}

void tst_QPipeWireCameraDeviceMonitor::objectAdded_deviceAfterNode_nodeBorrowsDescription()
{
    QCameraDeviceMonitor monitor;
    const DictFixture node{ PwPropertyDict{
            { "media.class", "Video/Source" },
            { "device.api", "v4l2" },
            { "node.name", "cam0" },
            { "device.id", "5" },
    } };
    const DictFixture device = deviceProperties("Integrated Camera");

    monitor.objectAdded(ObjectId{ 1 }, PipewireRegistryType::Node, node.dict());
    QCOMPARE(monitor.cameraNodes().front().deviceProperties.empty(), true);

    monitor.objectAdded(ObjectId{ 5 }, PipewireRegistryType::Device, device.dict());

    const std::vector nodes = monitor.cameraNodes();
    QCOMPARE(nodes.size(), 1);
    QCOMPARE(cameraDescription(nodes.front().properties, nodes.front().deviceProperties),
             u"Integrated Camera"_s);
}

void tst_QPipeWireCameraDeviceMonitor::objectAdded_deviceBeforeNode_nodeBorrowsDescription()
{
    QCameraDeviceMonitor monitor;
    const DictFixture device = deviceProperties("Integrated Camera");
    const DictFixture node{ PwPropertyDict{
            { "media.class", "Video/Source" },
            { "device.api", "v4l2" },
            { "node.name", "cam0" },
            { "device.id", "5" },
    } };

    monitor.objectAdded(ObjectId{ 5 }, PipewireRegistryType::Device, device.dict());
    monitor.objectAdded(ObjectId{ 1 }, PipewireRegistryType::Node, node.dict());

    const std::vector nodes = monitor.cameraNodes();
    QCOMPARE(nodes.size(), 1);
    QCOMPARE(cameraDescription(nodes.front().properties, nodes.front().deviceProperties),
             u"Integrated Camera"_s);
}

void tst_QPipeWireCameraDeviceMonitor::objectRemoved_removesNode()
{
    QCameraDeviceMonitor monitor;
    const DictFixture node = v4l2NodeProperties("cam0");
    monitor.objectAdded(ObjectId{ 1 }, PipewireRegistryType::Node, node.dict());
    QVERIFY(!monitor.cameraNodes().empty());

    monitor.objectRemoved(ObjectId{ 1 });

    QVERIFY(monitor.cameraNodes().empty());
}

void tst_QPipeWireCameraDeviceMonitor::
        objectRemoved_deviceRemoval_clearsBorrowedDescriptionOnSurvivingNode()
{
    QCameraDeviceMonitor monitor;
    const DictFixture node{ PwPropertyDict{
            { "media.class", "Video/Source" },
            { "device.api", "v4l2" },
            { "node.name", "cam0" },
            { "device.id", "5" },
    } };
    const DictFixture device = deviceProperties("Integrated Camera");

    monitor.objectAdded(ObjectId{ 1 }, PipewireRegistryType::Node, node.dict());
    monitor.objectAdded(ObjectId{ 5 }, PipewireRegistryType::Device, device.dict());
    QCOMPARE(monitor.cameraNodes().front().deviceProperties.empty(), false);

    monitor.objectRemoved(ObjectId{ 5 });

    const std::vector nodes = monitor.cameraNodes();
    QCOMPARE(nodes.size(), 1);
    QCOMPARE(nodes.front().deviceProperties.empty(), true);
}

void tst_QPipeWireCameraDeviceMonitor::objectAdded_emitsCamerasChanged_forCameraNode()
{
    QCameraDeviceMonitor monitor;
    QSignalSpy spy(&monitor, &QCameraDeviceMonitor::camerasChanged);
    const DictFixture node = v4l2NodeProperties("cam0");

    monitor.objectAdded(ObjectId{ 1 }, PipewireRegistryType::Node, node.dict());

    QCOMPARE(spy.size(), 1);
}

void tst_QPipeWireCameraDeviceMonitor::objectAdded_doesNotEmitCamerasChanged_forNonCameraNode()
{
    QCameraDeviceMonitor monitor;
    QSignalSpy spy(&monitor, &QCameraDeviceMonitor::camerasChanged);
    const DictFixture audioNode{ PwPropertyDict{
            { "media.class", "Audio/Source" },
            { "device.api", "alsa" },
    } };

    monitor.objectAdded(ObjectId{ 1 }, PipewireRegistryType::Node, audioNode.dict());

    QCOMPARE(spy.size(), 0);
}

QTEST_GUILESS_MAIN(tst_QPipeWireCameraDeviceMonitor)

#include "tst_qpipewire_cameradevicemonitor.moc"

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_cameranode_p.h"

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using namespace Qt::StringLiterals;
using namespace std::string_view_literals;

namespace {

QByteArray toByteArray(std::string_view sv)
{
    return QByteArray(sv.data(), qsizetype(sv.size()));
}

QString toString(std::string_view sv)
{
    return QString::fromUtf8(sv.data(), qsizetype(sv.size()));
}

} // namespace

bool isCameraNode(const PwPropertyDict &nodeProperties)
{
    // An escape hatch for triaging a camera that we filter out but should not.
    static const bool acceptAll =
            qEnvironmentVariableIntValue("QT_PIPEWIRE_CAMERA_ACCEPT_ALL") != 0;

    const std::optional<std::string_view> mediaClass = getMediaClass(nodeProperties);
    if (!mediaClass)
        return false;

    if (mediaClass != "Video/Source"sv)
        return false;

    if (acceptAll)
        return true;

    if (getMediaRole(nodeProperties) == "Camera"sv)
        return true;

    const std::optional<std::string_view> deviceApi = getDeviceApi(nodeProperties);
    return deviceApi == "v4l2"sv || deviceApi == "libcamera"sv;
}

QByteArray cameraDeviceId(const PwPropertyDict &nodeProperties)
{
    // node.name is derived from the device's sysfs path: stable and unique
    if (auto nodeName = getNodeName(nodeProperties))
        return "pw:" + toByteArray(*nodeName);

    // object.path less stable, based on the devfs name, e.g. v4l2:/dev/video0
    if (auto objectPath = getObjectPath(nodeProperties))
        return "pw:" + toByteArray(*objectPath);

    // Last resort: not stable across runs, but better than an empty id, which
    // would make the device unusable.
    if (auto serial = getObjectSerial(nodeProperties))
        return "pw:serial:" + QByteArray::number(qulonglong(serial->value));

    return {};
}

QString cameraDescription(const PwPropertyDict &nodeProperties,
                          const PwPropertyDict &deviceProperties)
{
    if (auto description = getNodeDescription(nodeProperties))
        return toString(*description);
    if (auto nick = getNodeNick(nodeProperties))
        return toString(*nick);
    if (auto description = getDeviceDescription(deviceProperties))
        return toString(*description);
    if (auto nodeName = getNodeName(nodeProperties))
        return toString(*nodeName);
    return {};
}

QCameraDevice::Position cameraPosition(const PwPropertyDict &nodeProperties)
{
    // Only libcamera reports where a camera faces; v4l2 has no notion of it.
    if (getDeviceApi(nodeProperties) != "libcamera"sv)
        return QCameraDevice::UnspecifiedPosition;

    const auto location = nodeProperties.find("api.libcamera.location"sv);
    if (location == nodeProperties.end())
        return QCameraDevice::UnspecifiedPosition;

    if (location->second == "front"sv)
        return QCameraDevice::FrontFace;
    if (location->second == "back"sv)
        return QCameraDevice::BackFace;

    // "external" and anything unrecognised.
    return QCameraDevice::UnspecifiedPosition;
}

} // namespace QtPipeWire

QT_END_NAMESPACE

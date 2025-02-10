// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_PROPERTYDICT_H
#define QPIPEWIRE_PROPERTYDICT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qpipewire_support_p.h"

#include <QtCore/qglobal.h>
#include <QtCore/qspan.h>

#include <pipewire/pipewire.h>

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

// pw_properties
PwPropertiesHandle makeProperties(QSpan<const spa_dict_item>);

// PwPropertyDict
using PwPropertyDict = std::map<std::string, std::string, std::less<>>;

// parser
PwPropertyDict toPropertyDict(const spa_dict &);

// property getters
// PW_KEY_MEDIA_CLASS
std::optional<std::string_view> getMediaClass(const PwPropertyDict &);
// PW_KEY_NODE_NAME
std::optional<std::string_view> getNodeName(const PwPropertyDict &);
// PW_KEY_NODE_DESCRIPTION
std::optional<std::string_view> getNodeDescription(const PwPropertyDict &);
// PW_KEY_DEVICE_SYSFS_PATH
std::optional<std::string_view> getDeviceSysfsPath(const PwPropertyDict &);
// PW_KEY_DEVICE_NAME
std::optional<std::string_view> getDeviceName(const PwPropertyDict &);
// PW_KEY_DEVICE_DESCRIPTION
std::optional<std::string_view> getDeviceDescription(const PwPropertyDict &);
// PW_KEY_DEVICE_ID
std::optional<ObjectId> getDeviceId(const PwPropertyDict &);
// PW_KEY_OBJECT_SERIAL
std::optional<ObjectSerial> getObjectSerial(const PwPropertyDict &);

// PW_KEY_METADATA_NAME
std::optional<std::string_view> getMetadataName(const PwPropertyDict &);

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_PROPERTYDICT_H

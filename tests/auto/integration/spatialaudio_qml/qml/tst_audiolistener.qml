// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D.SpatialAudio
import QtTest

TestCase {
    name: "AudioListener"

    AudioListener { id: listener }

    function test_instantiation_and_position() {
        verify(listener !== undefined)
        listener.position = Qt.vector3d(10, 20, 30)
        compare(listener.position.x, 10)
        compare(listener.position.y, 20)
        compare(listener.position.z, 30)
    }

    function test_rotation_rw() {
        // assign a quaternion; QML provides Qt.quaternion
        listener.rotation = Qt.quaternion(0,0,0,1)
        verify(listener.rotation !== undefined)
    }
}

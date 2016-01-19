/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtTest 1.0
import QtMultimedia 5.4

TestCase {

    function test_0_globalObject() {
        verify(typeof QtMultimedia !== 'undefined');
    }

    function test_1_defaultCamera() {
        verify(typeof QtMultimedia.defaultCamera !== 'undefined');

        var camera = QtMultimedia.defaultCamera;
        compare(camera.deviceId, "othercamera", "deviceId");
        compare(camera.displayName, "othercamera desc", "displayName");
        compare(camera.position, Camera.UnspecifiedPosition, "position");
        compare(camera.orientation, 0, "orientation");
    }

    function test_2_availableCameras() {
        verify(typeof QtMultimedia.availableCameras !== 'undefined');
        compare(QtMultimedia.availableCameras.length, 2);

        var camera = QtMultimedia.availableCameras[0];
        compare(camera.deviceId, "backcamera", "deviceId");
        compare(camera.displayName, "backcamera desc", "displayName");
        compare(camera.position, Camera.BackFace, "position");
        compare(camera.orientation, 90, "orientation");

        camera = QtMultimedia.availableCameras[1];
        compare(camera.deviceId, "othercamera", "deviceId");
        compare(camera.displayName, "othercamera desc", "displayName");
        compare(camera.position, Camera.UnspecifiedPosition, "position");
        compare(camera.orientation, 0, "orientation");
    }

}

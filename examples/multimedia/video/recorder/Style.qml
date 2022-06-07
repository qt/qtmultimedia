// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma Singleton
import QtQuick

QtObject {

    function isMobile() {
        return Qt.platform.os === "android" || Qt.platform.os === "ios";
    }

    function calculateRatio(windowWidth, windowHeight) {
        var refWidth = 800.;
        Style.ratio = windowWidth/refWidth;
    }

    property real ratio : 1

    property real screenWidth: isMobile()? 400 : 800
    property real screenHeigth: isMobile()? 900 : 600

    property real height: 25
    property real widthTiny: 40*ratio
    property real widthShort: 80*ratio
    property real widthMedium: 160*ratio
    property real widthLong: 220*ratio
    property real intraSpacing: 5
    property real interSpacing: 15
    property real fontSize: 11
}

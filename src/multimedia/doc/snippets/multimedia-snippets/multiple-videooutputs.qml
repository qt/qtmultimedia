/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

import QtQuick
import QtQuick.Window
import QtMultimedia

//! [complete]
Item {
    MediaPlayer {
        id: mediaplayer
        source: "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"
        videoOutput: [v1, v2]
        audioOutput: AudioOutput {

        }
    }

    VideoOutput {
        id: v1
        anchors.fill: parent
    }

    Window {
        visible: true
        width: 480; height: 320
        VideoOutput {
            id: v2
            anchors.fill: parent
        }
    }
}
//! [complete]

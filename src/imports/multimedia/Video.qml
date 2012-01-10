/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtMultimedia 5.0

Item {
    id: video

    /*** Properties of VideoOutput ***/
    property alias fillMode:            videoOut.fillMode
    property alias orientation:         videoOut.orientation

    readonly property int stretch:             VideoOutput.Stretch
    readonly property int preserveAspectFit:   VideoOutput.PreserveAspectFit
    readonly property int preserveAspectCrop:  VideoOutput.PreserveAspectCrop


    /*** Properties of MediaPlayer ***/
    property alias autoLoad:        player.autoLoad
    property alias bufferProgress:  player.bufferProgress
    property alias duration:        player.duration
    property alias error:           player.error
    property alias errorString:     player.errorString
    property alias metaData:        player.metaData
    property alias muted:           player.muted
    property alias paused:          player.paused
    property alias playbackRate:    player.playbackRate
    property alias playing:         player.playing
    property alias position:        player.position
    property alias seekable:        player.seekable
    property alias source:          player.source
    property alias status:          player.status
    property alias volume:          player.volume

    signal resumed
    signal started
    signal stopped


    VideoOutput {
        id: videoOut
        anchors.fill: video
        source: player
    }

    MediaPlayer {
        id: player
        onResumed: video.resumed()
        onStarted: video.started()
        onStopped: video.stopped()
    }

    function play() {
        player.play();
    }

    function pause() {
        player.pause();
    }

    function stop() {
        player.stop();
    }

}

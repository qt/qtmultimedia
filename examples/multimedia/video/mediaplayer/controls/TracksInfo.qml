// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    property alias model: model
    property int selectedTrack: 0

    function read(tracks, key = 0) {
        // language is the 6th index in the enum QMediaMetaData::Key
        model.clear()

        if (!tracks)
            return

        tracks.forEach((metadata, index) => {
            const data = metadata.stringValue(key)
            const label = data ? data : qsTr("track ") + (index + 1)
            model.append({data: label, index: index})
        })
    }

    ListModel { id: model }
}

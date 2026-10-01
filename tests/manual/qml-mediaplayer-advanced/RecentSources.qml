// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtCore
import QtQuick

// Remembers the sources that have been played, most recent first, across application
// launches.
QtObject {
    id: root

    readonly property int maximumCount: 20

    // List of source URLs as strings, most recent first.
    readonly property var sources: {
        try {
            const parsed = JSON.parse(storage.sourcesJson)
            return Array.isArray(parsed) ? parsed : []
        } catch (e) {
            return []
        }
    }

    // Moves the source to the front of the list, adding it if needed.
    function add(source) {
        const sourceString = String(source)
        if (sourceString === "")
            return
        const updated = [sourceString]
            .concat(root.sources.filter((item) => item !== sourceString))
            .slice(0, root.maximumCount)
        storage.sourcesJson = JSON.stringify(updated)
    }

    function clear() {
        storage.sourcesJson = "[]"
    }

    // Settings stores every property declared on it, so it only holds the stored data.
    property Settings settings: Settings {
        id: storage

        category: "RecentSources"

        // Stored as JSON, since a list with a single entry can come back from QSettings
        // as a plain string, depending on the storage format.
        property string sourcesJson: "[]"
    }
}

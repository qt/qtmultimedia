// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    property alias metadata: listModel
    property alias count: listModel.count

    function clear() {
        listModel.clear()
    }

    //! [0]
    function read(metadata) {
        if (!metadata)
            return
        for (const key of metadata.keys())
            if (metadata.stringValue(key))
                listModel.append({
                                    name: metadata.metaDataKeyToString(key),
                                    value: metadata.stringValue(key)
                                })
    }

    ListModel {
        id: listModel
    }
    //! [0]
}

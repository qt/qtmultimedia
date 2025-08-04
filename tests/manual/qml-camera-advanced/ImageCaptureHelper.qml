// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia

// Includes helper for turning file format into string.
ImageCapture {

    onFileFormatChanged: {
        console.log("ImageCapture: New format is " + fileFormatToString(fileFormat))
    }

    readonly property var allFileFormats: [
        ImageCapture.UnspecifiedFormat,
        ImageCapture.JPEG,
        ImageCapture.PNG,
        ImageCapture.WebP,
        ImageCapture.Tiff]

    function fileFormatToString(format) {
        switch (format) {
            case ImageCapture.UnspecifiedFormat: return "UnspecifiedFormat";
            case ImageCapture.JPEG: return "JPEG";
            case ImageCapture.PNG: return "PNG";
            case ImageCapture.WebP: return "WebP";
            case ImageCapture.Tiff: return "Tiff";
            default: return "Unknown format";
        }
    }

    function isFormatSupported(input) {
        return supportedFormats.includes(input);
    }
}

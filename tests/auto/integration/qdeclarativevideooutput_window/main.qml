import QtQuick
import QtMultimedia

Item {
    width: 200
    height: 200
    VideoOutput {
        objectName: "videoOutput"
        x: 25; y: 50
        width: 150
        height: 100
    }
}

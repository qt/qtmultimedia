// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QAudioDevice>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QString>
#include <QTextStream>

int main(int argc, char *argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    QTextStream out(stdout);

    //! [Media Audio Input Device Enumeration]
    const QList<QAudioDevice> audioDevices = QMediaDevices::audioInputs();
    for (const QAudioDevice &device : audioDevices)
    {
        out << "ID: " << device.id() << Qt::endl;
        out << "Description: " << device.description() << Qt::endl;
        out << "Is default: " << (device.isDefault() ? "Yes" : "No") << Qt::endl;
    }
    //! [Media Audio Input Device Enumeration]

    //! [Media Video Input Device Enumeration]
    const QList<QCameraDevice> videoDevices = QMediaDevices::videoInputs();
    for (const QCameraDevice &device : videoDevices)
    {
        out << "ID: " << device.id() << Qt::endl;
        out << "Description: " << device.description() << Qt::endl;
        out << "Is default: " << (device.isDefault() ? "Yes" : "No") << Qt::endl;
    }
    //! [Media Video Input Device Enumeration]

    return 0;
}

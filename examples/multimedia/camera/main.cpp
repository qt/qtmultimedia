// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "camera.h"

#include <QApplication>

int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    // To be removed after QTBUG-132816
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
#endif
    QApplication app(argc, argv);

    Camera camera;
    camera.show();

    return app.exec();
};

// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtWidgets/qapplication.h>
#include <QtCore/qloggingcategory.h>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QLoggingCategory::setFilterRules("*.debug=false\n"
                                     "qt.multimedia.wasm.*=true");
    MainWindow w;
    w.show();
    return a.exec();
}

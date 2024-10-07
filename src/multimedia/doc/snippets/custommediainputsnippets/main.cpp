// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "custommediainputsnippets.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    CustomMediaInputSnippets snippets;

    snippets.setupAndRecordVideo();
    snippets.setupAndRecordAudio();

    return 0;
}

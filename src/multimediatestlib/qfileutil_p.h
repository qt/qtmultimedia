// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QFILEUTIL_P_H
#define QFILEUTIL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qdir.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/qurl.h>

#include <memory>

QT_BEGIN_NAMESPACE

bool copyAllFiles(const QDir &source, const QDir &dest);

std::unique_ptr<QTemporaryFile> copyResourceToTemporaryFile(const QUrl &resource,
                                                            const QString &fileTemplate);

QT_END_NAMESPACE

#endif

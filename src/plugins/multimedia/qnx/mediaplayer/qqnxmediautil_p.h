// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef MMRENDERERUTIL_H
#define MMRENDERERUTIL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtMultimedia/qaudio.h>

typedef struct mmr_context mmr_context_t;

QT_BEGIN_NAMESPACE

class QString;

QString mmErrorMessage(const QString &msg, mmr_context_t *context, int * errorCode = 0);

bool checkForDrmPermission();

QT_END_NAMESPACE

#endif

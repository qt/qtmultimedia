// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDMULTIMEDIAUTILS_H
#define ANDROIDMULTIMEDIAUTILS_H

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

#include <qobject.h>

QT_BEGIN_NAMESPACE

class AndroidMultimediaUtils
{
public:
    enum MediaType {
        Music = 0,
        Movies = 1,
        DCIM = 2,
        Sounds = 3
    };

    static void enableOrientationListener(bool enable);
    static int getDeviceOrientation();
    static QString getDefaultMediaDirectory(MediaType type);
    static void registerMediaFile(const QString &file);
};

QT_END_NAMESPACE

#endif // ANDROIDMULTIMEDIAUTILS_H

// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVIDEOTRANSFORMATION_P_H
#define QVIDEOTRANSFORMATION_P_H

#include <qtvideo.h>
#include <optional>

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

QT_BEGIN_NAMESPACE

struct VideoTransformation
{
    QtVideo::Rotation rotation = QtVideo::Rotation::None;
    int rotationIndex = 0; // to be removed
    bool mirrorredHorizontallyAfterRotation = false;
};

using VideoTransformationOpt = std::optional<VideoTransformation>;

inline bool operator==(const VideoTransformation &lhs, const VideoTransformation &rhs)
{
    return lhs.rotation == rhs.rotation
            && lhs.mirrorredHorizontallyAfterRotation == rhs.mirrorredHorizontallyAfterRotation;
}

inline bool operator!=(const VideoTransformation &lhs, const VideoTransformation &rhs)
{
    return !(lhs == rhs);
}

QT_END_NAMESPACE

#endif // QVIDEOTRANSFORMATION_P_H

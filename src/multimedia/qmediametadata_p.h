// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QMEDIAMETADATA_P_H
#define QMEDIAMETADATA_P_H

#include <QtMultimedia/qmediametadata.h>
#include <QtGui/qimage.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

#if QT_DEPRECATED_SINCE(6, 12)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
constexpr auto deprecatedThumbnailImage = QMediaMetaData::Key::ThumbnailImage;
QT_WARNING_POP
#endif

void setCoverArtImage(QMediaMetaData &metadata, const QImage &image)
{
    if (image.isNull())
        return;

    metadata.insert(QMediaMetaData::CoverArtImage, image);
#if QT_DEPRECATED_SINCE(6, 12)
    metadata.insert(deprecatedThumbnailImage, image);
#endif
}

} // namespace QtMultimediaPrivate
QT_END_NAMESPACE

#endif // QMEDIAMETADATA_P_H

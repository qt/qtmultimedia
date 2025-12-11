// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVIDEOWIDGET_P_H
#define QVIDEOWIDGET_P_H

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

#include <QtMultimediaWidgets/qtmultimediawidgetsglobal.h>

#include <QtMultimediaWidgets/qvideowidget.h>
#include <QtMultimedia/qvideoframe.h>

QT_BEGIN_NAMESPACE

class QVideoWindow;

class QVideoWidgetPrivate
{
    Q_DECLARE_PUBLIC(QVideoWidget)
public:
    QVideoWidget *q_ptr = nullptr;
    Qt::WindowFlags nonFullScreenFlags;
    bool wasFullScreen = false;
    QPoint nonFullscreenPos;

    QVideoWindow *videoWindow = nullptr;
    QVideoSink *fakeVideoSink = nullptr;
    QWidget *windowContainer = nullptr;
    bool isEglfs = false;
};

QT_END_NAMESPACE

#endif

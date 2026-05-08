// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKMEDIAPLAYER_H
#define QQUICKMEDIAPLAYER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimediaQuick/qtmultimediaquickexports.h>
#include <QtQml/qqml.h>
#include <QtCore/qurl.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIAQUICK_EXPORT QQuickMediaPlayer : public QMediaPlayer
{
    Q_OBJECT

    Q_PROPERTY(bool autoPlay READ autoPlay WRITE setAutoPlay NOTIFY autoPlayChanged FINAL)

    QML_NAMED_ELEMENT(MediaPlayer)

public:
    QQuickMediaPlayer(QObject *parent = nullptr);

    bool autoPlay() const;
    void setAutoPlay(bool autoPlay);

private:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

Q_SIGNALS:
    void autoPlayChanged(bool autoPlay);

private:
    bool m_autoPlay = false;
    bool m_wasMediaLoaded = false;
};

QT_END_NAMESPACE

#endif

// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QMLVIDEOSINGLETON_H
#define QMLVIDEOSINGLETON_H

#include "qmlvideo_global.h"

#include <QtQml/qqml.h>

class QMLVIDEO_LIB_EXPORT VideoSingleton : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source1 READ source1 WRITE setSource1 NOTIFY source1Changed FINAL)
    Q_PROPERTY(QUrl source2 READ source2 WRITE setSource2 NOTIFY source2Changed FINAL)
    Q_PROPERTY(QUrl videoPath READ videoPath WRITE setVideoPath NOTIFY videoPathChanged FINAL)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged FINAL)
    QML_SINGLETON
    QML_ELEMENT

public:
    explicit VideoSingleton(QObject *parent = nullptr);

    QUrl source1() const;
    void setSource1(const QUrl &source1);
    QUrl source2() const;
    void setSource2(const QUrl &source2);
    QUrl videoPath() const;
    void setVideoPath(const QUrl &videoPath);
    qreal volume() const;
    void setVolume(qreal volume);

Q_SIGNALS:
    void source1Changed();
    void source2Changed();
    void volumeChanged();
    void videoPathChanged();

private:
    QUrl m_source1;
    QUrl m_source2;
    QUrl m_videoPath;
    qreal m_volume = 0.5;
};

QML_DECLARE_TYPE(VideoSingleton);

#endif // QMLVIDEOSINGLETON_H

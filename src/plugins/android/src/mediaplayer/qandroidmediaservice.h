/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QANDROIDMEDIASERVICE_H
#define QANDROIDMEDIASERVICE_H

#include <QMediaService>

QT_BEGIN_NAMESPACE

class QAndroidMediaPlayerControl;
class QAndroidMetaDataReaderControl;
class QAndroidAudioRoleControl;
class QAndroidCustomAudioRoleControl;
class QAndroidMediaPlayerVideoRendererControl;

class QAndroidMediaService : public QMediaService
{
    Q_OBJECT
public:
    explicit QAndroidMediaService(QObject *parent = 0);
    ~QAndroidMediaService() override;

    QMediaControl* requestControl(const char *name) override;
    void releaseControl(QMediaControl *control) override;

private:
    QAndroidMediaPlayerControl *mMediaControl;
    QAndroidMetaDataReaderControl *mMetadataControl;
    QAndroidAudioRoleControl *mAudioRoleControl;
    QAndroidCustomAudioRoleControl *mCustomAudioRoleControl;
    QAndroidMediaPlayerVideoRendererControl *mVideoRendererControl;
};

QT_END_NAMESPACE

#endif // QANDROIDMEDIASERVICE_H

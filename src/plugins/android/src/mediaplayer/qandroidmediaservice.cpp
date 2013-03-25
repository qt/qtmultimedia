/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidmediaservice.h"

#include "qandroidmediaplayercontrol.h"
#include "qandroidmetadatareadercontrol.h"
#include "qandroidvideorendercontrol.h"

QT_BEGIN_NAMESPACE

QAndroidMediaService::QAndroidMediaService(QObject *parent)
    : QMediaService(parent)
    , mVideoRendererControl(0)
{
    mMediaControl = new QAndroidMediaPlayerControl;
    mMetadataControl = new QAndroidMetaDataReaderControl;
    connect(mMediaControl, SIGNAL(mediaChanged(QMediaContent)),
            mMetadataControl, SLOT(onMediaChanged(QMediaContent)));
    connect(mMediaControl, SIGNAL(metaDataUpdated()),
            mMetadataControl, SLOT(onUpdateMetaData()));
}

QAndroidMediaService::~QAndroidMediaService()
{
    delete mMediaControl;
    delete mMetadataControl;
    delete mVideoRendererControl;
}

QMediaControl *QAndroidMediaService::requestControl(const char *name)
{
    if (qstrcmp(name, QMediaPlayerControl_iid) == 0)
        return mMediaControl;

    if (qstrcmp(name, QMetaDataReaderControl_iid) == 0)
        return mMetadataControl;

    if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
        if (!mVideoRendererControl) {
            mVideoRendererControl = new QAndroidVideoRendererControl;
            mMediaControl->setVideoOutput(mVideoRendererControl);
            return mVideoRendererControl;
        }
    }

    return 0;
}

void QAndroidMediaService::releaseControl(QMediaControl *control)
{
    if (control == mVideoRendererControl) {
        mMediaControl->setVideoOutput(0);
        delete mVideoRendererControl;
        mVideoRendererControl = 0;
    }
}

QT_END_NAMESPACE

/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AVFMEDIARECORDERCONTROL_IOS_H
#define AVFMEDIARECORDERCONTROL_IOS_H

#include "avfmediaassetwriter.h"
#include "avfstoragelocation.h"
#include "avfcamerautility.h"

#include <QtMultimedia/qmediarecordercontrol.h>

#include <QtCore/qglobal.h>
#include <QtCore/qurl.h>

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFCameraService;
class QString;
class QUrl;

class AVFMediaRecorderControlIOS : public QMediaRecorderControl, public AVFMediaAssetWriterDelegate
{
    Q_OBJECT
public:
    AVFMediaRecorderControlIOS(AVFCameraService *service, QObject *parent = 0);
    ~AVFMediaRecorderControlIOS();

    QUrl outputLocation() const Q_DECL_OVERRIDE;
    bool setOutputLocation(const QUrl &location) Q_DECL_OVERRIDE;

    QMediaRecorder::State state() const Q_DECL_OVERRIDE;
    QMediaRecorder::Status status() const Q_DECL_OVERRIDE;

    qint64 duration() const Q_DECL_OVERRIDE;

    bool isMuted() const Q_DECL_OVERRIDE;
    qreal volume() const Q_DECL_OVERRIDE;

    void applySettings() Q_DECL_OVERRIDE;

public Q_SLOTS:
    void setState(QMediaRecorder::State state) Q_DECL_OVERRIDE;
    void setMuted(bool muted) Q_DECL_OVERRIDE;
    void setVolume(qreal volume) Q_DECL_OVERRIDE;

    // Writer delegate:
private:

    void assetWriterStarted() Q_DECL_OVERRIDE;
    void assetWriterFailedToStart() Q_DECL_OVERRIDE;
    void assetWriterFailedToStop() Q_DECL_OVERRIDE;
    void assetWriterFinished() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void captureModeChanged(QCamera::CaptureModes);
    void cameraStatusChanged(QCamera::Status newStatus);

private:
    void stopWriter();

    AVFCameraService *m_service;

    AVFScopedPointer<dispatch_queue_t> m_writerQueue;
    AVFScopedPointer<QT_MANGLE_NAMESPACE(AVFMediaAssetWriter)> m_writer;

    QUrl m_outputLocation;
    AVFStorageLocation m_storageLocation;

    QMediaRecorder::State m_state;
    QMediaRecorder::Status m_lastStatus;
};

QT_END_NAMESPACE

#endif // AVFMEDIARECORDERCONTROL_IOS_H

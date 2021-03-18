/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AVFMEDIARECORDERCONTROL_IOS_H
#define AVFMEDIARECORDERCONTROL_IOS_H

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

#include "avfmediaassetwriter_p.h"
#include "avfstoragelocation_p.h"
#include "avfcamerautility_p.h"
#include "qaudiodeviceinfo.h"

#include <private/qplatformmediaencoder_p.h>
#include <private/qvideooutputorientationhandler_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/qurl.h>

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFCameraService;
class QString;
class QUrl;

class AVFMediaRecorderControl : public QPlatformMediaEncoder
{
    Q_OBJECT
public:
    AVFMediaRecorderControl(AVFCameraService *service, QObject *parent = nullptr);
    ~AVFMediaRecorderControl() override;

    QUrl outputLocation() const override;
    bool setOutputLocation(const QUrl &location) override;

    QMediaEncoder::State state() const override;
    QMediaEncoder::Status status() const override;

    qint64 duration() const override;

    void applySettings() override;
    void unapplySettings();

    void setEncoderSettings(const QMediaEncoderSettings &settings) override;
    QMediaEncoderSettings encoderSettings() const {
        QMediaEncoderSettings s = m_settings;
        s.resolveFormat();
        return s;
    }

    AVFCameraService *cameraService() const { return m_service; }

public Q_SLOTS:
    void setState(QMediaEncoder::State state) override;

private:

    Q_INVOKABLE void assetWriterStarted();
    Q_INVOKABLE void assetWriterFinished();

private Q_SLOTS:
    void cameraStatusChanged(QCamera::Status newStatus);

private:
    void stopWriter();

    AVFCameraService *m_service;
    AVFScopedPointer<QT_MANGLE_NAMESPACE(AVFMediaAssetWriter)> m_writer;

    QUrl m_outputLocation;
    AVFStorageLocation m_storageLocation;

    QMediaEncoder::State m_state;
    QMediaEncoder::Status m_lastStatus;
    QMediaEncoderSettings m_settings;

    NSDictionary *m_audioSettings;
    NSDictionary *m_videoSettings;
    QVideoOutputOrientationHandler m_orientationHandler;
};

QT_END_NAMESPACE

#endif // AVFMEDIARECORDERCONTROL_IOS_H

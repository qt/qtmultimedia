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


#ifndef QFFMPEGMEDIARECODER_H
#define QFFMPEGMEDIARECODER_H

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

#include <private/qplatformmediarecorder_p.h>
#include "qffmpegmediacapturesession_p.h"

#include "qffmpeg_p.h"

QT_BEGIN_NAMESPACE

class QAudioSource;
class QAudioSourceIO;
class QAudioBuffer;
class QMediaMetaData;

namespace QFFmpeg {
class Encoder;
}

class QFFmpegMediaRecorder : public QObject, public QPlatformMediaRecorder
{
    Q_OBJECT
public:
    QFFmpegMediaRecorder(QMediaRecorder *parent);
    virtual ~QFFmpegMediaRecorder();

    bool isLocationWritable(const QUrl &sink) const override;

    void record(QMediaEncoderSettings &settings) override;
    void pause() override;
    void resume() override;
    void stop() override;

    void setMetaData(const QMediaMetaData &) override;
    QMediaMetaData metaData() const override;

    void setCaptureSession(QPlatformMediaCaptureSession *session);

private Q_SLOTS:
    void newDuration(qint64 d) { durationChanged(d); }
    void finalizationDone();
    void handleSessionError(QMediaRecorder::Error code, const QString &description);

private:
    QFFmpegMediaCaptureSession *m_session = nullptr;
    QMediaMetaData m_metaData;

    QFFmpeg::Encoder *encoder = nullptr;
};

QT_END_NAMESPACE

#endif // QFFMPEGMEDIARECODER_H

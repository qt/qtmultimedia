/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QFFMPEGAUDIODECODER_H
#define QFFMPEGAUDIODECODER_H

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

#include "private/qplatformaudiodecoder_p.h"
#include <qffmpeg_p.h>

#include <qmutex.h>
#include <qurl.h>
#include <qqueue.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {
class Decoder;
}

class QFFmpegAudioDecoder : public QPlatformAudioDecoder
{
    Q_OBJECT

public:
    QFFmpegAudioDecoder(QAudioDecoder *parent);
    virtual ~QFFmpegAudioDecoder();

    QUrl source() const override;
    void setSource(const QUrl &fileName) override;

    QIODevice *sourceDevice() const override;
    void setSourceDevice(QIODevice *device) override;

    void start() override;
    void stop() override;

    QAudioFormat audioFormat() const override;
    void setAudioFormat(const QAudioFormat &format) override;

    QAudioBuffer read() override;
    bool bufferAvailable() const override;

    qint64 position() const override;
    qint64 duration() const override;

private:
    QUrl m_url;
    QIODevice *m_sourceDevice = nullptr;
    QFFmpeg::Decoder *decoder = nullptr;
    QAudioFormat m_audioFormat;

    mutable QMutex queueMutex;
    QQueue<QAudioBuffer> queue;
    qint64 m_position = 0;
    qint64 m_duration = 0;
};

QT_END_NAMESPACE

#endif // QFFMPEGAUDIODECODER_H

/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MFAUDIODECODERCONTROL_H
#define MFAUDIODECODERCONTROL_H

#include "qaudiodecodercontrol.h"
#include "mfdecodersourcereader.h"
#include "sourceresolver.h"

QT_USE_NAMESPACE

class MFAudioDecoderControl : public QAudioDecoderControl
{
    Q_OBJECT
public:
    MFAudioDecoderControl(QObject *parent = 0);
    ~MFAudioDecoderControl();

    QAudioDecoder::State state() const;

    QString sourceFilename() const;
    void setSourceFilename(const QString &fileName);

    QIODevice* sourceDevice() const;
    void setSourceDevice(QIODevice *device);

    void start();
    void stop();

    QAudioFormat audioFormat() const;
    void setAudioFormat(const QAudioFormat &format);

    QAudioBuffer read();
    bool bufferAvailable() const;

    qint64 position() const;
    qint64 duration() const;

private Q_SLOTS:
    void handleMediaSourceReady();
    void handleMediaSourceError(long hr);
    void handleSampleAdded();
    void handleSourceFinished();

private:
    void updateResamplerOutputType();
    void activatePipeline();
    void onSourceCleared();

    MFDecoderSourceReader  *m_decoderSourceReader;
    SourceResolver         *m_sourceResolver;
    IMFTransform           *m_resampler;
    QAudioDecoder::State    m_state;
    QString                 m_sourceFilename;
    QIODevice              *m_device;
    QAudioFormat            m_audioFormat;
    DWORD                   m_mfInputStreamID;
    DWORD                   m_mfOutputStreamID;
    bool                    m_bufferReady;
    QAudioBuffer            m_cachedAudioBuffer;
    qint64                  m_duration;
    qint64                  m_position;
    bool                    m_loadingSource;
    IMFMediaType           *m_mfOutputType;
    IMFSample              *m_convertSample;
    QAudioFormat            m_sourceOutputFormat;
    bool                    m_sourceReady;
    bool                    m_resamplerDirty;
};

#endif//MFAUDIODECODERCONTROL_H

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

#ifndef MOCKAUDIODECODERCONTROL_H
#define MOCKAUDIODECODERCONTROL_H

#include "qmediacontrol.h"
#include <private/qaudiodecodercontrol_p.h>

#include <QtCore/qpair.h>

#include "qaudiobuffer.h"
#include <QTimer>
#include <QIODevice>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Multimedia)

class MockAudioDecoderControl : public QAudioDecoderControl
{
    Q_OBJECT

public:
    MockAudioDecoderControl(QObject *parent = 0)
        : QAudioDecoderControl(parent)
        , mState(QAudioDecoder::StoppedState)
        , mDevice(0)
        , mSerial(0)
    {
        mFormat.setChannels(1);
        mFormat.setSampleSize(8);
        mFormat.setFrequency(1000);
        mFormat.setCodec("audio/x-raw");
        mFormat.setSampleType(QAudioFormat::UnSignedInt);
    }

    QAudioDecoder::State state() const
    {
        return mState;
    }

    QString sourceFilename() const
    {
        return mSource;
    }

    QAudioFormat audioFormat() const
    {
        return mFormat;
    }

    void setAudioFormat(const QAudioFormat &format)
    {
        if (mFormat != format) {
            mFormat = format;
            emit formatChanged(mFormat);
        }
    }

    void setSourceFilename(const QString &fileName)
    {
        mSource = fileName;
        mDevice = 0;
        stop();
    }

    QIODevice* sourceDevice() const
    {
        return mDevice;
    }

    void setSourceDevice(QIODevice *device)
    {
        mDevice = device;
        mSource.clear();
        stop();
    }

    // When decoding we decode to first buffer, then second buffer
    // we then stop until the first is read again and so on, for
    // 5 buffers
    void start()
    {
        if (mState == QAudioDecoder::StoppedState) {
            if (!mSource.isEmpty()) {
                mState = QAudioDecoder::DecodingState;
                emit stateChanged(mState);

                QTimer::singleShot(50, this, SLOT(pretendDecode()));
            } else {
                emit error(QAudioDecoder::ResourceError, "No source set");
            }
        }
    }

    void stop()
    {
        if (mState != QAudioDecoder::StoppedState) {
            mState = QAudioDecoder::StoppedState;
            mSerial = 0;
            mBuffers.clear();
            emit stateChanged(mState);
            emit bufferAvailableChanged(false);
        }
    }

    QAudioBuffer read(bool *ok)
    {
        if (mBuffers.length() > 0) {
            if (ok)
                *ok = true;
            QAudioBuffer a = mBuffers.takeFirst();
            if (mBuffers.length() == 0)
                emit bufferAvailableChanged(false);
            QTimer::singleShot(50, this, SLOT(pretendDecode()));
            return a;
        } else {
            // Can't do anything here :(
            if (ok)
                *ok = false;
            return QAudioBuffer();
        }
    }

    bool bufferAvailable() const
    {
        return mBuffers.length() > 0;
    }

private slots:
    void pretendDecode()
    {
        // We just keep the length of mBuffers to 3 or less.
        if (mBuffers.length() < 3) {
            QByteArray b(sizeof(mSerial), 0);
            memcpy(b.data(), &mSerial, sizeof(mSerial));
            mSerial++;
            mBuffers.push_back(QAudioBuffer(b, mFormat));
            emit bufferReady();
            if (mBuffers.count() == 1)
                emit bufferAvailableChanged(true);
        } else {
            // Can't do anything here, wait for a read to restart the timer
            mState = QAudioDecoder::WaitingState;
            emit stateChanged(mState);
        }
    }

public:
    QAudioDecoder::State mState;
    QString mSource;
    QIODevice *mDevice;
    QAudioFormat mFormat;

    int mSerial;
    QList<QAudioBuffer> mBuffers;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif  // QAUDIODECODERCONTROL_H

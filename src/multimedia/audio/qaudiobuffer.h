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

#ifndef QAUDIOBUFFER_H
#define QAUDIOBUFFER_H

#include <QSharedDataPointer>

#include <qtmultimediadefs.h>
#include <qtmedianamespace.h>

#include <qaudio.h>
#include <qaudioformat.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Multimedia)

class QAbstractAudioBuffer;
class QAudioBufferPrivate;
class Q_MULTIMEDIA_EXPORT QAudioBuffer
{
public:
    QAudioBuffer();
    QAudioBuffer(QAbstractAudioBuffer *provider);
    QAudioBuffer(const QAudioBuffer &other);
    QAudioBuffer(const QByteArray &data, const QAudioFormat &format, qint64 startTime = -1);
    QAudioBuffer(int numFrames, const QAudioFormat &format, qint64 startTime = -1); // Initialized to empty

    QAudioBuffer& operator=(const QAudioBuffer &other);

    ~QAudioBuffer();

    bool isValid() const;

    QAudioFormat format() const;

    int frameCount() const;
    int sampleCount() const;
    int byteCount() const;

    qint64 duration() const;
    qint64 startTime() const;

    // Data modification
    // void clear();
    // Other ideas
    // operator *=
    // operator += (need to be careful about different formats)

    // Data access
    const void* constData() const; // Does not detach, preferred
    const void* data() const; // Does not detach
    void *data(); // detaches

    // Structures for easier access to stereo data
    template <typename T> struct StereoFrameDefault { enum { Default = 0 }; };

    template <typename T> struct StereoFrame {

        StereoFrame()
            : left(T(StereoFrameDefault<T>::Default))
            , right(T(StereoFrameDefault<T>::Default))
        {
        }

        StereoFrame(T leftSample, T rightSample)
            : left(leftSample)
            , right(rightSample)
        {
        }

        StereoFrame& operator=(const StereoFrame &other)
        {
            // Two straight assigns is probably
            // cheaper than a conditional check on
            // self assignment
            left = other.left;
            right = other.right;
            return *this;
        }

        T left;
        T right;

        T average() const {return (left + right) / 2;}
        void clear() {left = right = T(StereoFrameDefault<T>::Default);}
    };

    typedef StereoFrame<unsigned char> S8U;
    typedef StereoFrame<signed char> S8S;
    typedef StereoFrame<unsigned short> S16U;
    typedef StereoFrame<signed short> S16S;
    typedef StereoFrame<float> S32F;

    template <typename T> const T* constData() const {
        return static_cast<const T*>(constData());
    }
    template <typename T> const T* data() const {
        return static_cast<const T*>(data());
    }
    template <typename T> T* data() {
        return static_cast<T*>(data());
    }
private:
    QAudioBufferPrivate *d;
};

template <> struct QAudioBuffer::StereoFrameDefault<unsigned char> { enum { Default = 128 }; };
template <> struct QAudioBuffer::StereoFrameDefault<unsigned short> { enum { Default = 32768 }; };


QT_END_NAMESPACE

Q_DECLARE_METATYPE(QAudioBuffer)

QT_END_HEADER

#endif // QAUDIOBUFFER_H

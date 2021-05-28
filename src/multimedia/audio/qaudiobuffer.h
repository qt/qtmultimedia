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

#ifndef QAUDIOBUFFER_H
#define QAUDIOBUFFER_H

#include <QtCore/qshareddata.h>

#include <QtMultimedia/qtmultimediaglobal.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudioformat.h>

QT_BEGIN_NAMESPACE

class QAudioBufferPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QAudioBufferPrivate, Q_MULTIMEDIA_EXPORT)

class Q_MULTIMEDIA_EXPORT QAudioBuffer
{
public:
    QAudioBuffer() noexcept;
    QAudioBuffer(const QAudioBuffer &other) noexcept;
    QAudioBuffer(const QByteArray &data, const QAudioFormat &format, qint64 startTime = -1);
    QAudioBuffer(int numFrames, const QAudioFormat &format, qint64 startTime = -1); // Initialized to empty
    ~QAudioBuffer();

    QAudioBuffer& operator=(const QAudioBuffer &other);

    QAudioBuffer(QAudioBuffer &&other) noexcept = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QAudioBuffer)
    void swap(QAudioBuffer &other) noexcept
    { qSwap(d, other.d); }

    bool isValid() const noexcept { return d != nullptr; };

    void detach();

    QAudioFormat format() const noexcept;

    qsizetype frameCount() const noexcept;
    qsizetype sampleCount() const noexcept;
    qsizetype byteCount() const noexcept;

    qint64 duration() const noexcept;
    qint64 startTime() const noexcept;

    // Data access
    const void* constData() const noexcept;
    const void* data() const noexcept;
    void *data();

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
    QExplicitlySharedDataPointer<QAudioBufferPrivate> d;
};

template <> struct QAudioBuffer::StereoFrameDefault<unsigned char> { enum { Default = 128 }; };
template <> struct QAudioBuffer::StereoFrameDefault<unsigned short> { enum { Default = 32768 }; };

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QAudioBuffer)

#endif // QAUDIOBUFFER_H

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

#include "qandroidformatsinfo_p.h"

QT_BEGIN_NAMESPACE

QAndroidFormatInfo::QAndroidFormatInfo()
{
    // ### Properly determine the set of supported codecs, this is a minimal set gathered from the old code base
    m_decodableAudioCodecs << Qt::AudioCodec::AAC << Qt::AudioCodec::MP3;
    m_encodableAudioCodecs << Qt::AudioCodec::AAC << Qt::AudioCodec::MP3;
    m_decodableVideoCodecs << Qt::VideoCodec::MPEG4 << Qt::VideoCodec::H264;
    m_encodableVideoCodecs << Qt::VideoCodec::MPEG4 << Qt::VideoCodec::H264;
    m_decodableMediaContainers << Qt::MediaContainer::MPEG4 << Qt::MediaContainer::MP3;
    m_encodableMediaContainers << Qt::MediaContainer::MPEG4 << Qt::MediaContainer::MP3;
}

QAndroidFormatInfo::~QAndroidFormatInfo
{

}

QList<Qt::MediaContainer> QAndroidFormatInfo::decodableMediaContainers() const
{
    return m_decodableMediaContainers;
}

QList<Qt::AudioCodec> QAndroidFormatInfo::decodableAudioCodecs() const
{
    return m_decodableAudioCodecs;
}

QList<Qt::VideoCodec> QAndroidFormatInfo::decodableVideoCodecs() const
{
    return m_decodableVideoCodecs;
}

QList<Qt::MediaContainer> QAndroidFormatInfo::encodableMediaContainers() const
{
    return m_encodableMediaContainers;
}

QList<Qt::AudioCodec> QAndroidFormatInfo::encodableAudioCodecs() const
{
    return m_encodableAudioCodecs;
}

QList<Qt::VideoCodec> QAndroidFormatInfo::encodableVideoCodecs() const
{
    return m_encodableVideoCodecs;
}

QT_END_NAMESPACE

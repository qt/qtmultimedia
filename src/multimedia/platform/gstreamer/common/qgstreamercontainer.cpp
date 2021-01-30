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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include "qgstreamercontainer_p.h"
#include "qgstreameraudioencoder_p.h"
#include "qgstreamervideoencoder_p.h"
#include <private/qgstutils_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QGStreamerContainerControl::QGStreamerContainerControl(QObject *parent)
    :QMediaContainerControl(parent)
    , m_supportedContainers(QGstCodecsInfo::Muxer)
{
}

QStringList QGStreamerContainerControl::supportedContainers() const
{
    return m_supportedContainers.supportedCodecs();
}

QString QGStreamerContainerControl::containerDescription(const QString &formatMimeType) const
{
    return m_supportedContainers.codecDescription(formatMimeType);
}

QString QGStreamerContainerControl::containerFormat() const
{
    return m_format;
}

void QGStreamerContainerControl::setContainerFormat(const QString &format)
{
    if (m_format != format) {
        m_format = format;
        m_actualFormat = format;
        emit settingsChanged();
    }
}

QString QGStreamerContainerControl::actualContainerFormat() const
{
    return m_actualFormat;
}

void QGStreamerContainerControl::setActualContainerFormat(const QString &containerFormat)
{
    m_actualFormat = containerFormat;
}

void QGStreamerContainerControl::resetActualContainerFormat()
{
    m_actualFormat = m_format;
}

GstEncodingContainerProfile *QGStreamerContainerControl::createProfile()
{
    GstCaps *caps = nullptr;

    if (m_actualFormat.isEmpty())
        return 0;

    QString format = m_actualFormat;
    const QStringList supportedFormats = m_supportedContainers.supportedCodecs();

    //if format is not in the list of supported gstreamer mime types,
    //try to find the mime type with matching extension
    if (!supportedFormats.contains(format)) {
        format.clear();
        QString extension = QGstUtils::fileExtensionForMimeType(m_actualFormat);
        for (const QString &formatCandidate : supportedFormats) {
            if (QGstUtils::fileExtensionForMimeType(formatCandidate) == extension) {
                format = formatCandidate;
                break;
            }
        }
    }

    if (format.isEmpty())
        return nullptr;

    caps = gst_caps_from_string(format.toLatin1());

    GstEncodingContainerProfile *profile = (GstEncodingContainerProfile *)gst_encoding_container_profile_new(
                "camerabin2_profile",
                (gchar *)"custom camera profile",
                caps,
                NULL); //preset

    gst_caps_unref(caps);

    return profile;
}

GstEncodingContainerProfile *QGStreamerContainerControl::fullProfile(QGStreamerAudioEncoderControl *audioEncoderControl,
                                                                     QGStreamerVideoEncoderControl *videoEncoderControl)
{
    auto *containerProfile = createProfile();
    if (containerProfile) {
        GstEncodingProfile *audioProfile = audioEncoderControl ? audioEncoderControl->createProfile() : nullptr;
        GstEncodingProfile *videoProfile = videoEncoderControl ? videoEncoderControl->createProfile() : nullptr;
        qDebug() << "audio profile" << gst_caps_to_string(gst_encoding_profile_get_format(audioProfile));
        qDebug() << "video profile" << gst_caps_to_string(gst_encoding_profile_get_format(videoProfile));
        qDebug() << "conta profile" << gst_caps_to_string(gst_encoding_profile_get_format((GstEncodingProfile *)containerProfile));

        if (videoProfile) {
            if (!gst_encoding_container_profile_add_profile(containerProfile, videoProfile))
                gst_encoding_profile_unref(videoProfile);
        }
        if (audioProfile) {
            if (!gst_encoding_container_profile_add_profile(containerProfile, audioProfile))
                gst_encoding_profile_unref(audioProfile);
        }
    }

    return containerProfile;
}

void QGStreamerContainerControl::applySettings(QGStreamerAudioEncoderControl *audioEncoderControl, QGStreamerVideoEncoderControl *videoEncoderControl)
{
    resetActualContainerFormat();
    audioEncoderControl->resetActualSettings();
    videoEncoderControl->resetActualSettings();

    //encodebin doesn't like the encoding profile with ANY caps,
    //if container and codecs are not specified,
    //try to find a commonly used supported combination
    if (containerFormat().isEmpty() &&
            audioEncoderControl->audioSettings().codec().isEmpty() &&
            videoEncoderControl->videoSettings().codec().isEmpty()) {

        QList<QStringList> candidates;

        // By order of preference

        // .mp4 (h264, AAC)
        candidates.append(QStringList() << "video/quicktime, variant=(string)iso" << "video/x-h264" << "audio/mpeg, mpegversion=(int)4");

        // .mp4 (h264, AC3)
        candidates.append(QStringList() << "video/quicktime, variant=(string)iso" << "video/x-h264" << "audio/x-ac3");

        // .mp4 (h264, MP3)
        candidates.append(QStringList() << "video/quicktime, variant=(string)iso" << "video/x-h264" << "audio/mpeg, mpegversion=(int)1, layer=(int)3");

        // .mkv (h264, AAC)
        candidates.append(QStringList() << "video/x-matroska" << "video/x-h264" << "audio/mpeg, mpegversion=(int)4");

        // .mkv (h264, AC3)
        candidates.append(QStringList() << "video/x-matroska" << "video/x-h264" << "audio/x-ac3");

        // .mkv (h264, MP3)
        candidates.append(QStringList() << "video/x-matroska" << "video/x-h264" << "audio/mpeg, mpegversion=(int)1, layer=(int)3");

        // .mov (h264, AAC)
        candidates.append(QStringList() << "video/quicktime" << "video/x-h264" << "audio/mpeg, mpegversion=(int)4");

        // .mov (h264, MP3)
        candidates.append(QStringList() << "video/quicktime" << "video/x-h264" << "audio/mpeg, mpegversion=(int)1, layer=(int)3");

        // .webm (VP8, Vorbis)
        candidates.append(QStringList() << "video/webm" << "video/x-vp8" << "audio/x-vorbis");

        // .ogg (Theora, Vorbis)
        candidates.append(QStringList() << "application/ogg" << "video/x-theora" << "audio/x-vorbis");

        // .avi (DivX, MP3)
        candidates.append(QStringList() << "video/x-msvideo" << "video/x-divx" << "audio/mpeg, mpegversion=(int)1, layer=(int)3");

        for (const QStringList &candidate : qAsConst(candidates)) {
            if (supportedContainers().contains(candidate[0]) &&
                    videoEncoderControl->supportedVideoCodecs().contains(candidate[1]) &&
                    audioEncoderControl->supportedAudioCodecs().contains(candidate[2])) {
                setActualContainerFormat(candidate[0]);

                QVideoEncoderSettings videoSettings = videoEncoderControl->videoSettings();
                videoSettings.setCodec(candidate[1]);
                videoEncoderControl->setActualVideoSettings(videoSettings);

                QAudioEncoderSettings audioSettings = audioEncoderControl->audioSettings();
                audioSettings.setCodec(candidate[2]);
                audioEncoderControl->setActualAudioSettings(audioSettings);

                break;
            }
        }
    }
}

QSet<QString> QGStreamerContainerControl::supportedStreamTypes(const QString &container) const
{
    return m_supportedContainers.supportedStreamTypes(container);
}

QString QGStreamerContainerControl::containerExtension() const
{
    return QGstUtils::fileExtensionForMimeType(m_format);
}


QT_END_NAMESPACE

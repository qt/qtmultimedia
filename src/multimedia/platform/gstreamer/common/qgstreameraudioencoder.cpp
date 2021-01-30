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
#include "qgstreameraudioencoder_p.h"
#include "qgstreamercontainer_p.h"
#include <private/qgstutils_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QGStreamerAudioEncoderControl::QGStreamerAudioEncoderControl(QObject *parent)
    : QAudioEncoderSettingsControl(parent)
    , m_codecs(QGstCodecsInfo::AudioEncoder)
{
}

QGStreamerAudioEncoderControl::~QGStreamerAudioEncoderControl()
{
}

QStringList QGStreamerAudioEncoderControl::supportedAudioCodecs() const
{
    return m_codecs.supportedCodecs();
}

QString QGStreamerAudioEncoderControl::codecDescription(const QString &codecName) const
{
    return m_codecs.codecDescription(codecName);
}

QList<int> QGStreamerAudioEncoderControl::supportedSampleRates(const QAudioEncoderSettings &, bool *) const
{
    //TODO check element caps to find actual values

    return QList<int>();
}

QAudioEncoderSettings QGStreamerAudioEncoderControl::audioSettings() const
{
    return m_audioSettings;
}

void QGStreamerAudioEncoderControl::setAudioSettings(const QAudioEncoderSettings &settings)
{
    if (m_audioSettings != settings) {
        m_audioSettings = settings;
        m_actualAudioSettings = settings;
        emit settingsChanged();
    }
}

QAudioEncoderSettings QGStreamerAudioEncoderControl::actualAudioSettings() const
{
    return m_actualAudioSettings;
}

void QGStreamerAudioEncoderControl::setActualAudioSettings(const QAudioEncoderSettings &settings)
{
    m_actualAudioSettings = settings;
}

void QGStreamerAudioEncoderControl::resetActualSettings()
{
    m_actualAudioSettings = m_audioSettings;
}

GstEncodingProfile *QGStreamerAudioEncoderControl::createProfile()
{
    QString codec = m_actualAudioSettings.codec();
    QString preset = m_actualAudioSettings.encodingOption(QStringLiteral("preset")).toString();
    GstCaps *caps;

    if (codec.isEmpty())
        return 0;

    caps = gst_caps_from_string(codec.toLatin1());

    GstEncodingProfile *profile = (GstEncodingProfile *)gst_encoding_audio_profile_new(
                caps,
                !preset.isEmpty() ? preset.toLatin1().constData() : NULL, //preset
                NULL,   //restriction
                0);     //presence

    gst_caps_unref(caps);

    return profile;
}

void QGStreamerAudioEncoderControl::applySettings(GstElement *encoder)
{
    GObjectClass * const objectClass = G_OBJECT_GET_CLASS(encoder);
    const char * const name = qt_gst_element_get_factory_name(encoder);

    const bool isVorbis = qstrcmp(name, "vorbisenc") == 0;

    const int bitRate = m_actualAudioSettings.bitRate();
    if (!isVorbis && bitRate == -1) {
        // Bit rate is invalid, don't evaluate the remaining conditions unless the encoder is
        // vorbisenc which is known to accept -1 as an unspecified bitrate.
    } else if (g_object_class_find_property(objectClass, "bitrate")) {
        g_object_set(G_OBJECT(encoder), "bitrate", bitRate, NULL);
    } else if (g_object_class_find_property(objectClass, "target-bitrate")) {
        g_object_set(G_OBJECT(encoder), "target-bitrate", bitRate, NULL);
    }

    if (isVorbis) {
        static const double qualities[] = { 0.1, 0.3, 0.5, 0.7, 1.0 };
        g_object_set(G_OBJECT(encoder), "quality", qualities[m_actualAudioSettings.quality()], NULL);
    }
}


QSet<QString> QGStreamerAudioEncoderControl::supportedStreamTypes(const QString &codecName) const
{
    return m_codecs.supportedStreamTypes(codecName);
}

GstElement *QGStreamerAudioEncoderControl::createEncoder()
{
    QString codec = m_audioSettings.codec();
    GstElement *encoderElement = gst_element_factory_make(m_codecs.codecElement(codec).constData(), NULL);
    if (!encoderElement)
        return 0;

    GstBin * encoderBin = GST_BIN(gst_bin_new("audio-encoder-bin"));

    GstElement *sinkCapsFilter = gst_element_factory_make("capsfilter", NULL);
    GstElement *srcCapsFilter = gst_element_factory_make("capsfilter", NULL);

    gst_bin_add_many(encoderBin, sinkCapsFilter, encoderElement, srcCapsFilter, NULL);
    gst_element_link_many(sinkCapsFilter, encoderElement, srcCapsFilter, NULL);

    // add ghostpads
    GstPad *pad = gst_element_get_static_pad(sinkCapsFilter, "sink");
    gst_element_add_pad(GST_ELEMENT(encoderBin), gst_ghost_pad_new("sink", pad));
    gst_object_unref(GST_OBJECT(pad));

    pad = gst_element_get_static_pad(srcCapsFilter, "src");
    gst_element_add_pad(GST_ELEMENT(encoderBin), gst_ghost_pad_new("src", pad));
    gst_object_unref(GST_OBJECT(pad));

    if (m_audioSettings.sampleRate() > 0 || m_audioSettings.channelCount() > 0) {
        GstCaps *caps = gst_caps_new_empty();
        GstStructure *structure = qt_gst_structure_new_empty("audio/x-raw");

        if (m_audioSettings.sampleRate() > 0)
            gst_structure_set(structure, "rate", G_TYPE_INT, m_audioSettings.sampleRate(), NULL );

        if (m_audioSettings.channelCount() > 0)
            gst_structure_set(structure, "channels", G_TYPE_INT, m_audioSettings.channelCount(), NULL );

        gst_caps_append_structure(caps,structure);

        g_object_set(G_OBJECT(sinkCapsFilter), "caps", caps, NULL);

        gst_caps_unref(caps);
    }

    // Some encoders support several codecs. Setting a caps filter downstream with the desired
    // codec (which is actually a string representation of the caps) will make sure we use the
    // correct codec.
    GstCaps *caps = gst_caps_from_string(codec.toUtf8().constData());
    g_object_set(G_OBJECT(srcCapsFilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    if (encoderElement) {
        if (m_audioSettings.encodingMode() == QMultimedia::ConstantQualityEncoding) {
            QMultimedia::EncodingQuality qualityValue = m_audioSettings.quality();

            if (codec == QLatin1String("audio/x-vorbis")) {
                double qualityTable[] = {
                    0.1, //VeryLow
                    0.3, //Low
                    0.5, //Normal
                    0.7, //High
                    1.0 //VeryHigh
                };
                g_object_set(G_OBJECT(encoderElement), "quality", qualityTable[qualityValue], NULL);
            } else if (codec == QLatin1String("audio/mpeg")) {
                g_object_set(G_OBJECT(encoderElement), "target", 0, NULL); //constant quality mode
                qreal quality[] = {
                    1, //VeryLow
                    3, //Low
                    5, //Normal
                    7, //High
                    9 //VeryHigh
                };
                g_object_set(G_OBJECT(encoderElement), "quality", quality[qualityValue], NULL);
            } else if (codec == QLatin1String("audio/x-speex")) {
                //0-10 range with default 8
                double qualityTable[] = {
                    2, //VeryLow
                    5, //Low
                    8, //Normal
                    9, //High
                    10 //VeryHigh
                };
                g_object_set(G_OBJECT(encoderElement), "quality", qualityTable[qualityValue], NULL);
            } else if (codec.startsWith("audio/AMR")) {
                int band[] = {
                    0, //VeryLow
                    2, //Low
                    4, //Normal
                    6, //High
                    7  //VeryHigh
                };

                g_object_set(G_OBJECT(encoderElement), "band-mode", band[qualityValue], NULL);
            }
        } else {
            int bitrate = m_audioSettings.bitRate();
            if (bitrate > 0) {
                if (codec == QLatin1String("audio/mpeg")) {
                    g_object_set(G_OBJECT(encoderElement), "target", 1, NULL); //constant bitrate mode
                }
                g_object_set(G_OBJECT(encoderElement), "bitrate", bitrate, NULL);
            }
        }

#if 0
        QMap<QString, QVariant> options = m_options.value(codec);
        for (auto it = options.cbegin(), end = options.cend(); it != end; ++it) {
            const QString &option = it.key();
            const QVariant &value = it.value();

            switch (value.typeId()) {
            case QMetaType::Int:
                g_object_set(G_OBJECT(encoderElement), option.toLatin1(), value.toInt(), NULL);
                break;
            case QMetaType::Bool:
                g_object_set(G_OBJECT(encoderElement), option.toLatin1(), value.toBool(), NULL);
                break;
            case QMetaType::Double:
                g_object_set(G_OBJECT(encoderElement), option.toLatin1(), value.toDouble(), NULL);
                break;
            case QMetaType::QString:
                g_object_set(G_OBJECT(encoderElement), option.toLatin1(), value.toString().toUtf8().constData(), NULL);
                break;
            default:
                qWarning() << "unsupported option type:" << option << value;
                break;
            }

        }
#endif
    }

    return GST_ELEMENT(encoderBin);
}

QT_END_NAMESPACE

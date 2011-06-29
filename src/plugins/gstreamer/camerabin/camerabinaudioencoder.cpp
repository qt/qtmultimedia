/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "camerabinaudioencoder.h"
#include "camerabincontainer.h"

#include <QtCore/qdebug.h>

CameraBinAudioEncoder::CameraBinAudioEncoder(QObject *parent)
    :QAudioEncoderControl(parent)
{
    QList<QByteArray> codecCandidates;

#if defined(Q_WS_MAEMO_5) || defined(Q_WS_MAEMO_6)
    codecCandidates << "audio/AAC" << "audio/PCM" << "audio/AMR" << "audio/AMR-WB" << "audio/speex"
                    << "audio/ADPCM" << "audio/iLBC" << "audio/vorbis" << "audio/mpeg" << "audio/FLAC";

    m_elementNames["audio/AAC"] = "nokiaaacenc";
    m_elementNames["audio/speex"] = "speexenc";
    m_elementNames["audio/PCM"] = "audioresample";
    m_elementNames["audio/AMR"] = "nokiaamrnbenc";
    m_elementNames["audio/AMR-WB"] = "nokiaamrwbenc";
    m_elementNames["audio/ADPCM"] = "nokiaadpcmenc";
    m_elementNames["audio/iLBC"] = "nokiailbcenc";
    m_elementNames["audio/vorbis"] = "vorbisenc";
    m_elementNames["audio/FLAC"] = "flacenc";
    m_elementNames["audio/mpeg"] = "ffenc_mp2";
#else
    codecCandidates << "audio/mpeg" << "audio/vorbis" << "audio/speex" << "audio/GSM"
                    << "audio/PCM" << "audio/AMR" << "audio/AMR-WB";

    m_elementNames["audio/mpeg"] = "lamemp3enc";
    m_elementNames["audio/vorbis"] = "vorbisenc";
    m_elementNames["audio/speex"] = "speexenc";
    m_elementNames["audio/GSM"] = "gsmenc";
    m_elementNames["audio/PCM"] = "audioresample";
    m_elementNames["audio/AMR"] = "amrnbenc";
    m_elementNames["audio/AMR-WB"] = "amrwbenc";

    m_codecOptions["audio/vorbis"] = QStringList() << "min-bitrate" << "max-bitrate";
    m_codecOptions["audio/mpeg"] = QStringList() << "mode";
    m_codecOptions["audio/speex"] = QStringList() << "mode" << "vbr" << "vad" << "dtx";
    m_codecOptions["audio/GSM"] = QStringList();
    m_codecOptions["audio/PCM"] = QStringList();
    m_codecOptions["audio/AMR"] = QStringList();
    m_codecOptions["audio/AMR-WB"] = QStringList();
#endif

    foreach( const QByteArray& codecName, codecCandidates ) {
        QByteArray elementName = m_elementNames[codecName];
        GstElementFactory *factory = gst_element_factory_find(elementName.constData());

        if (factory) {
            m_codecs.append(codecName);
            const gchar *descr = gst_element_factory_get_description(factory);

            if (codecName == QByteArray("audio/PCM"))
                m_codecDescriptions.insert(codecName, tr("Raw PCM audio"));
            else
                m_codecDescriptions.insert(codecName, QString::fromUtf8(descr));

            m_streamTypes.insert(codecName,
                                 CameraBinContainer::supportedStreamTypes(factory, GST_PAD_SRC));

            gst_object_unref(GST_OBJECT(factory));
        }
    }
}

CameraBinAudioEncoder::~CameraBinAudioEncoder()
{
}

QStringList CameraBinAudioEncoder::supportedAudioCodecs() const
{
    return m_codecs;
}

QString CameraBinAudioEncoder::codecDescription(const QString &codecName) const
{
    return m_codecDescriptions.value(codecName);
}

QStringList CameraBinAudioEncoder::supportedEncodingOptions(const QString &codec) const
{
    return m_codecOptions.value(codec);
}

QVariant CameraBinAudioEncoder::encodingOption(
        const QString &codec, const QString &name) const
{
    return m_options[codec].value(name);
}

void CameraBinAudioEncoder::setEncodingOption(
        const QString &codec, const QString &name, const QVariant &value)
{
    m_options[codec][name] = value;
}

QList<int> CameraBinAudioEncoder::supportedSampleRates(const QAudioEncoderSettings &, bool *) const
{
    //TODO check element caps to find actual values

    return QList<int>();
}

QAudioEncoderSettings CameraBinAudioEncoder::audioSettings() const
{
    return m_audioSettings;
}

void CameraBinAudioEncoder::setAudioSettings(const QAudioEncoderSettings &settings)
{
    m_userSettings = settings;
    m_audioSettings = settings;
    emit settingsChanged();
}

void CameraBinAudioEncoder::setActualAudioSettings(const QAudioEncoderSettings &settings)
{
    m_audioSettings = settings;
}

void CameraBinAudioEncoder::resetActualSettings()
{
    m_audioSettings = m_userSettings;
}

GstElement *CameraBinAudioEncoder::createEncoder()
{
    QString codec = m_audioSettings.codec();
    QByteArray encoderElementName = m_elementNames.value(codec);
    GstElement *encoderElement = gst_element_factory_make(encoderElementName.constData(), NULL);
    if (!encoderElement)
        return 0;

    GstBin * encoderBin = GST_BIN(gst_bin_new("audio-encoder-bin"));
    GstElement *capsFilter = gst_element_factory_make("capsfilter", NULL);

    gst_bin_add(encoderBin, capsFilter);
    gst_bin_add(encoderBin, encoderElement);
    gst_element_link(capsFilter, encoderElement);

    // add ghostpads
    GstPad *pad = gst_element_get_static_pad(capsFilter, "sink");
    gst_element_add_pad(GST_ELEMENT(encoderBin), gst_ghost_pad_new("sink", pad));
    gst_object_unref(GST_OBJECT(pad));

    pad = gst_element_get_static_pad(encoderElement, "src");
    gst_element_add_pad(GST_ELEMENT(encoderBin), gst_ghost_pad_new("src", pad));
    gst_object_unref(GST_OBJECT(pad));

    if (m_audioSettings.sampleRate() > 0 || m_audioSettings.channelCount() > 0) {
        GstCaps *caps = gst_caps_new_empty();
        GstStructure *structure = gst_structure_new("audio/x-raw-int", NULL);

        if (m_audioSettings.sampleRate() > 0)
            gst_structure_set(structure, "rate", G_TYPE_INT, m_audioSettings.sampleRate(), NULL );

        if (m_audioSettings.channelCount() > 0)
            gst_structure_set(structure, "channels", G_TYPE_INT, m_audioSettings.channelCount(), NULL );

        gst_caps_append_structure(caps,structure);

        g_object_set(G_OBJECT(capsFilter), "caps", caps, NULL);
    }

    if (encoderElement) {
        if (m_audioSettings.encodingMode() == QtMultimediaKit::ConstantQualityEncoding) {
            QtMultimediaKit::EncodingQuality qualityValue = m_audioSettings.quality();

            if (encoderElementName == "lamemp3enc") {
                g_object_set(G_OBJECT(encoderElement), "target", 0, NULL); //constant quality mode
                qreal quality[] = {
                    10.0, //VeryLow
                    6.0, //Low
                    4.0, //Normal
                    2.0, //High
                    0.0 //VeryHigh
                };
                g_object_set(G_OBJECT(encoderElement), "quality", quality[qualityValue], NULL);
            } else if (encoderElementName == "ffenc_mp2") {
                int quality[] = {
                    8000, //VeryLow
                    64000, //Low
                    128000, //Normal
                    192000, //High
                    320000 //VeryHigh
                };
                g_object_set(G_OBJECT(encoderElement), "bitrate", quality[qualityValue], NULL);
            } else if (codec == QLatin1String("audio/speex")) {
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
                g_object_set(G_OBJECT(encoderElement), "bitrate", bitrate, NULL);
            }
        }

        QMap<QString, QVariant> options = m_options.value(codec);
        QMapIterator<QString,QVariant> it(options);
        while (it.hasNext()) {
            it.next();
            QString option = it.key();
            QVariant value = it.value();

            switch (value.type()) {
            case QVariant::Int:
                g_object_set(G_OBJECT(encoderElement), option.toAscii(), value.toInt(), NULL);
                break;
            case QVariant::Bool:
                g_object_set(G_OBJECT(encoderElement), option.toAscii(), value.toBool(), NULL);
                break;
            case QVariant::Double:
                g_object_set(G_OBJECT(encoderElement), option.toAscii(), value.toDouble(), NULL);
                break;
            case QVariant::String:
                g_object_set(G_OBJECT(encoderElement), option.toAscii(), value.toString().toUtf8().constData(), NULL);
                break;
            default:
                qWarning() << "unsupported option type:" << option << value;
                break;
            }
        }
    }

    return GST_ELEMENT(encoderBin);

}


QSet<QString> CameraBinAudioEncoder::supportedStreamTypes(const QString &codecName) const
{
    return m_streamTypes.value(codecName);
}

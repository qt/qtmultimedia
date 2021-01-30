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
#include "qgstreamervideoencoder_p.h"
#include "qgstreamercontainer_p.h"
#include <private/qgstutils_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QGStreamerVideoEncoderControl::QGStreamerVideoEncoderControl()
    : QVideoEncoderSettingsControl()
    , m_codecs(QGstCodecsInfo::VideoEncoder)
{
}

QGStreamerVideoEncoderControl::~QGStreamerVideoEncoderControl()
{
}

QStringList QGStreamerVideoEncoderControl::supportedVideoCodecs() const
{
    return m_codecs.supportedCodecs();
}

QString QGStreamerVideoEncoderControl::videoCodecDescription(const QString &codecName) const
{
    return m_codecs.codecDescription(codecName);
}

QVideoEncoderSettings QGStreamerVideoEncoderControl::videoSettings() const
{
    return m_videoSettings;
}

void QGStreamerVideoEncoderControl::setVideoSettings(const QVideoEncoderSettings &settings)
{
    if (m_videoSettings != settings) {
        m_actualVideoSettings = settings;
        m_videoSettings = settings;
        emit settingsChanged();
    }
}

QVideoEncoderSettings QGStreamerVideoEncoderControl::actualVideoSettings() const
{
    return m_actualVideoSettings;
}

void QGStreamerVideoEncoderControl::setActualVideoSettings(const QVideoEncoderSettings &settings)
{
    m_actualVideoSettings = settings;
}

void QGStreamerVideoEncoderControl::resetActualSettings()
{
    m_actualVideoSettings = m_videoSettings;
}


QPair<int,int> QGStreamerVideoEncoderControl::rateAsRational(qreal frameRate) const
{
    if (frameRate < 0)
        frameRate = m_actualVideoSettings.frameRate();

    if (frameRate > 0.001) {
        //convert to rational number
        QList<int> denumCandidates;
        denumCandidates << 1 << 2 << 3 << 5 << 10 << 25 << 30 << 50 << 100 << 1001 << 1000;

        qreal error = 1.0;
        int num = 1;
        int denum = 1;

        for (int curDenum : qAsConst(denumCandidates)) {
            int curNum = qRound(frameRate*curDenum);
            qreal curError = qAbs(qreal(curNum)/curDenum - frameRate);

            if (curError < error) {
                error = curError;
                num = curNum;
                denum = curDenum;
            }

            if (curError < 1e-8)
                break;
        }

        return QPair<int,int>(num,denum);
    }

    return QPair<int,int>();
}

GstEncodingProfile *QGStreamerVideoEncoderControl::createProfile()
{
    QString codec = m_actualVideoSettings.codec();
    GstCaps *caps = !codec.isEmpty() ? gst_caps_from_string(codec.toLatin1()) : nullptr;

    if (!caps)
        return nullptr;

    QString preset = m_actualVideoSettings.encodingOption(QStringLiteral("preset")).toString();
    GstEncodingVideoProfile *profile = gst_encoding_video_profile_new(
                caps,
                !preset.isEmpty() ? preset.toLatin1().constData() : NULL, //preset
                NULL, //restriction
                0); //presence

    gst_caps_unref(caps);

    gst_encoding_video_profile_set_pass(profile, 0);
    gst_encoding_video_profile_set_variableframerate(profile, TRUE);

    return (GstEncodingProfile *)profile;
}

void QGStreamerVideoEncoderControl::applySettings(GstElement *encoder)
{
    GObjectClass * const objectClass = G_OBJECT_GET_CLASS(encoder);
    const char * const name = qt_gst_element_get_factory_name(encoder);

    const int bitRate = m_actualVideoSettings.bitRate();
    if (bitRate == -1) {
        // Bit rate is invalid, don't evaluate the remaining conditions.
    } else if (g_object_class_find_property(objectClass, "bitrate")) {
        g_object_set(G_OBJECT(encoder), "bitrate", bitRate, NULL);
    } else if (g_object_class_find_property(objectClass, "target-bitrate")) {
        g_object_set(G_OBJECT(encoder), "target-bitrate", bitRate, NULL);
    }

    if (qstrcmp(name, "theoraenc") == 0) {
        static const int qualities[] = { 8, 16, 32, 45, 60 };
        g_object_set(G_OBJECT(encoder), "quality", qualities[m_actualVideoSettings.quality()], NULL);
    } else if (qstrncmp(name, "avenc_", 6) == 0) {
        if (g_object_class_find_property(objectClass, "pass")) {
            static const int modes[] = { 0, 2, 512, 1024 };
            g_object_set(G_OBJECT(encoder), "pass", modes[m_actualVideoSettings.encodingMode()], NULL);
        }
        if (g_object_class_find_property(objectClass, "quantizer")) {
            static const double qualities[] = { 20, 8.0, 3.0, 2.5, 2.0 };
            g_object_set(G_OBJECT(encoder), "quantizer", qualities[m_actualVideoSettings.quality()], NULL);
        }
    } else if (qstrncmp(name, "omx", 3) == 0) {
        if (!g_object_class_find_property(objectClass, "control-rate")) {
        } else switch (m_actualVideoSettings.encodingMode()) {
        case QMultimedia::ConstantBitRateEncoding:
            g_object_set(G_OBJECT(encoder), "control-rate", 2, NULL);
            break;
        case QMultimedia::AverageBitRateEncoding:
            g_object_set(G_OBJECT(encoder), "control-rate", 1, NULL);
            break;
        default:
            g_object_set(G_OBJECT(encoder), "control-rate", 0, NULL);
        }
    }
}


QSet<QString> QGStreamerVideoEncoderControl::supportedStreamTypes(const QString &codecName) const
{
    return m_codecs.supportedStreamTypes(codecName);
}


GstElement *QGStreamerVideoEncoderControl::createEncoder()
{
    QString codec = m_videoSettings.codec();
    GstElement *encoderElement = gst_element_factory_make(m_codecs.codecElement(codec).constData(), "video-encoder");
    if (!encoderElement)
        return 0;

    GstBin *encoderBin = GST_BIN(gst_bin_new("video-encoder-bin"));

    GstElement *sinkCapsFilter = gst_element_factory_make("capsfilter", "capsfilter-video-in");
    GstElement *srcCapsFilter = gst_element_factory_make("capsfilter", "capsfilter-video-out");
    gst_bin_add_many(encoderBin, sinkCapsFilter, srcCapsFilter, NULL);

    GstElement *colorspace = gst_element_factory_make("videoconvert", NULL);
    gst_bin_add(encoderBin, colorspace);
    gst_bin_add(encoderBin, encoderElement);

    gst_element_link_many(sinkCapsFilter, colorspace, encoderElement, srcCapsFilter, NULL);

    // add ghostpads
    GstPad *pad = gst_element_get_static_pad(sinkCapsFilter, "sink");
    gst_element_add_pad(GST_ELEMENT(encoderBin), gst_ghost_pad_new("sink", pad));
    gst_object_unref(GST_OBJECT(pad));

    pad = gst_element_get_static_pad(srcCapsFilter, "src");
    gst_element_add_pad(GST_ELEMENT(encoderBin), gst_ghost_pad_new("src", pad));
    gst_object_unref(GST_OBJECT(pad));

    if (encoderElement) {
        if (m_videoSettings.encodingMode() == QMultimedia::ConstantQualityEncoding) {
            QMultimedia::EncodingQuality qualityValue = m_videoSettings.quality();

            if (codec == QLatin1String("video/x-h264")) {
                //constant quantizer mode
                g_object_set(G_OBJECT(encoderElement), "pass", 4, NULL);
                int qualityTable[] = {
                    50, //VeryLow
                    35, //Low
                    21, //Normal
                    15, //High
                    8 //VeryHigh
                };
                g_object_set(G_OBJECT(encoderElement), "quantizer", qualityTable[qualityValue], NULL);
            } else if (codec == QLatin1String("video/x-xvid")) {
                //constant quantizer mode
                g_object_set(G_OBJECT(encoderElement), "pass", 3, NULL);
                int qualityTable[] = {
                    32, //VeryLow
                    12, //Low
                    5, //Normal
                    3, //High
                    2 //VeryHigh
                };
                int quant = qualityTable[qualityValue];
                g_object_set(G_OBJECT(encoderElement), "quantizer", quant, NULL);
            } else if (codec.startsWith(QLatin1String("video/mpeg"))) {
                //constant quantizer mode
                g_object_set(G_OBJECT(encoderElement), "pass", 2, NULL);
                //quant from 1 to 30, default ~3
                double qualityTable[] = {
                    20, //VeryLow
                    8.0, //Low
                    3.0, //Normal
                    2.5, //High
                    2.0 //VeryHigh
                };
                double quant = qualityTable[qualityValue];
                g_object_set(G_OBJECT(encoderElement), "quantizer", quant, NULL);
            } else if (codec == QLatin1String("video/x-theora")) {
                int qualityTable[] = {
                    8, //VeryLow
                    16, //Low
                    32, //Normal
                    45, //High
                    60 //VeryHigh
                };
                //quality from 0 to 63
                int quality = qualityTable[qualityValue];
                g_object_set(G_OBJECT(encoderElement), "quality", quality, NULL);
            }
        } else {
            int bitrate = m_videoSettings.bitRate();
            if (bitrate > 0) {
                g_object_set(G_OBJECT(encoderElement), "bitrate", bitrate, NULL);
            }
        }

#if 0
        QMap<QString,QVariant> options = m_options.value(codec);
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

    if (!m_videoSettings.resolution().isEmpty() || m_videoSettings.frameRate() > 0.001) {
        GstCaps *caps = QGstUtils::videoFilterCaps();

        if (!m_videoSettings.resolution().isEmpty()) {
            gst_caps_set_simple(
                        caps,
                        "width", G_TYPE_INT, m_videoSettings.resolution().width(),
                        "height", G_TYPE_INT, m_videoSettings.resolution().height(),
                        NULL);
        }

        if (m_videoSettings.frameRate() > 0.001) {
            QPair<int,int> rate = rateAsRational();
            gst_caps_set_simple(
                        caps,
                        "framerate", GST_TYPE_FRACTION, rate.first, rate.second,
                        NULL);
        }

        //qDebug() << "set video caps filter:" << gst_caps_to_string(caps);

        g_object_set(G_OBJECT(sinkCapsFilter), "caps", caps, NULL);

        gst_caps_unref(caps);
    }

    // Some encoders support several codecs. Setting a caps filter downstream with the desired
    // codec (which is actually a string representation of the caps) will make sure we use the
    // correct codec.
    GstCaps *caps = gst_caps_from_string(codec.toUtf8().constData());
    g_object_set(G_OBJECT(srcCapsFilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    return GST_ELEMENT(encoderBin);
}

QT_END_NAMESPACE

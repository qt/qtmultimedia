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

#include "camerabinvideoencoder.h"
#include "camerabinsession.h"
#include "camerabincontainer.h"

#include <QtCore/qdebug.h>

CameraBinVideoEncoder::CameraBinVideoEncoder(CameraBinSession *session)
    :QVideoEncoderControl(session), m_session(session)
{
    QList<QByteArray> codecCandidates;
#if defined(Q_WS_MAEMO_5)
    codecCandidates << "video/mpeg4" << "video/h264" << "video/h263" << "video/theora"
                    << "video/mpeg2" << "video/mpeg1" << "video/mjpeg" << "video/VP8" << "video/h261";

    m_elementNames["video/h264"] = "dsph264enc";
    m_elementNames["video/mpeg4"] = "dspmp4venc";
    m_elementNames["video/h263"] = "dsph263enc";
    m_elementNames["video/theora"] = "theoraenc";
    m_elementNames["video/mpeg2"] = "ffenc_mpeg2video";
    m_elementNames["video/mpeg1"] = "ffenc_mpeg1video";
    m_elementNames["video/mjpeg"] = "ffenc_mjpeg";
    m_elementNames["video/VP8"] = "vp8enc";
    m_elementNames["video/h261"] = "ffenc_h261";

    m_codecOptions["video/mpeg4"] = QStringList() << "mode" << "keyframe-interval";
#elif defined(Q_WS_MAEMO_6)
    codecCandidates << "video/mpeg4" << "video/h264" << "video/h263";

    m_elementNames["video/h264"] = "dsph264enc";
    m_elementNames["video/mpeg4"] = "dsphdmp4venc";
    m_elementNames["video/h263"] = "dsph263enc";

    QStringList options = QStringList() << "mode" << "keyframe-interval" << "max-bitrate" << "intra-refresh";
    m_codecOptions["video/h264"] = options;
    m_codecOptions["video/mpeg4"] = options;
    m_codecOptions["video/h263"] = options;
#else
    codecCandidates << "video/h264" << "video/xvid" << "video/mpeg4"
                    << "video/mpeg1" << "video/mpeg2" << "video/theora"
                    << "video/VP8" << "video/h261" << "video/mjpeg";

    m_elementNames["video/h264"] = "x264enc";
    m_elementNames["video/xvid"] = "xvidenc";
    m_elementNames["video/mpeg4"] = "ffenc_mpeg4";
    m_elementNames["video/mpeg1"] = "ffenc_mpeg1video";
    m_elementNames["video/mpeg2"] = "ffenc_mpeg2video";
    m_elementNames["video/theora"] = "theoraenc";
    m_elementNames["video/mjpeg"] = "ffenc_mjpeg";
    m_elementNames["video/VP8"] = "vp8enc";
    m_elementNames["video/h261"] = "ffenc_h261";

    m_codecOptions["video/h264"] = QStringList() << "quantizer";
    m_codecOptions["video/xvid"] = QStringList() << "quantizer" << "profile";
    m_codecOptions["video/mpeg4"] = QStringList() << "quantizer";
    m_codecOptions["video/mpeg1"] = QStringList() << "quantizer";
    m_codecOptions["video/mpeg2"] = QStringList() << "quantizer";
    m_codecOptions["video/theora"] = QStringList();

#endif

    foreach( const QByteArray& codecName, codecCandidates ) {
        QByteArray elementName = m_elementNames[codecName];
        GstElementFactory *factory = gst_element_factory_find(elementName.constData());
        if (factory) {
            m_codecs.append(codecName);
            const gchar *descr = gst_element_factory_get_description(factory);
            m_codecDescriptions.insert(codecName, QString::fromUtf8(descr));

            m_streamTypes.insert(codecName,
                                 CameraBinContainer::supportedStreamTypes(factory, GST_PAD_SRC));

            gst_object_unref(GST_OBJECT(factory));
        }
    }
}

CameraBinVideoEncoder::~CameraBinVideoEncoder()
{
}

QList<QSize> CameraBinVideoEncoder::supportedResolutions(const QVideoEncoderSettings &settings, bool *continuous) const
{
    if (continuous)
        *continuous = false;

    QPair<int,int> rate = rateAsRational(settings.frameRate());

    //select the closest supported rational rate to settings.frameRate()

    return m_session->supportedResolutions(rate, continuous, QCamera::CaptureVideo);
}

QList< qreal > CameraBinVideoEncoder::supportedFrameRates(const QVideoEncoderSettings &settings, bool *continuous) const
{
    if (continuous)
        *continuous = false;

    QList< qreal > res;
    QPair<int,int> rate;

    foreach(rate, m_session->supportedFrameRates(settings.resolution(), continuous)) {
        if (rate.second > 0)
            res << qreal(rate.first)/rate.second;
    }

    return res;
}

QStringList CameraBinVideoEncoder::supportedVideoCodecs() const
{
    return m_codecs;
}

QString CameraBinVideoEncoder::videoCodecDescription(const QString &codecName) const
{
    return m_codecDescriptions.value(codecName);
}

QStringList CameraBinVideoEncoder::supportedEncodingOptions(const QString &codec) const
{
    return m_codecOptions.value(codec);
}

QVariant CameraBinVideoEncoder::encodingOption(const QString &codec, const QString &name) const
{
    return m_options[codec].value(name);
}

void CameraBinVideoEncoder::setEncodingOption(
        const QString &codec, const QString &name, const QVariant &value)
{
    m_options[codec][name] = value;
}

QVideoEncoderSettings CameraBinVideoEncoder::videoSettings() const
{
    return m_videoSettings;
}

void CameraBinVideoEncoder::setVideoSettings(const QVideoEncoderSettings &settings)
{
    m_videoSettings = settings;
    m_userSettings = settings;
    emit settingsChanged();
}

void CameraBinVideoEncoder::setActualVideoSettings(const QVideoEncoderSettings &settings)
{
    m_videoSettings = settings;
}

void CameraBinVideoEncoder::resetActualSettings()
{
    m_videoSettings = m_userSettings;
}

GstElement *CameraBinVideoEncoder::createEncoder()
{
    QString codec = m_videoSettings.codec();
    QByteArray elementName = m_elementNames.value(codec);

    GstElement *encoderElement = gst_element_factory_make( elementName.constData(), "video-encoder");

    if (encoderElement) {
        if (m_videoSettings.encodingMode() == QtMultimediaKit::ConstantQualityEncoding) {
            QtMultimediaKit::EncodingQuality qualityValue = m_videoSettings.quality();

            if (elementName == "x264enc") {
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
            } else if (elementName == "xvidenc") {
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
            } else if (elementName == "ffenc_mpeg4" ||
                       elementName == "ffenc_mpeg1video" ||
                       elementName == "ffenc_mpeg2video" ) {
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
            } else if (elementName == "theoraenc") {
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
            } else if (elementName == "dsph264enc" ||
                       elementName == "dspmp4venc" ||
                       elementName == "dsphdmp4venc" ||
                       elementName == "dsph263enc") {
                //only bitrate parameter is supported
                int qualityTable[] = {
                    1000000, //VeryLow
                    2000000, //Low
                    4000000, //Normal
                    8000000, //High
                    16000000 //VeryHigh
                };
                int bitrate = qualityTable[qualityValue];
                g_object_set(G_OBJECT(encoderElement), "bitrate", bitrate, NULL);
            }
        } else {
            int bitrate = m_videoSettings.bitRate();
            if (bitrate > 0) {
                g_object_set(G_OBJECT(encoderElement), "bitrate", bitrate, NULL);
            }
        }

        QMap<QString,QVariant> options = m_options.value(codec);
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

    return encoderElement;
}

QPair<int,int> CameraBinVideoEncoder::rateAsRational(qreal frameRate) const
{
    if (frameRate > 0.001) {
        //convert to rational number
        QList<int> denumCandidates;
        denumCandidates << 1 << 2 << 3 << 5 << 10 << 25 << 30 << 50 << 100 << 1001 << 1000;

        qreal error = 1.0;
        int num = 1;
        int denum = 1;

        foreach (int curDenum, denumCandidates) {
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


QSet<QString> CameraBinVideoEncoder::supportedStreamTypes(const QString &codecName) const
{
    return m_streamTypes.value(codecName);
}

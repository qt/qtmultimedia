// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMediaRecorder_H
#define QMediaRecorder_H

#include <QtCore/qobject.h>
#include <QtCore/qsize.h>
#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmediaenumdebug.h>
#include <QtMultimedia/qmediametadata.h>

#include <QtCore/qpair.h>

QT_BEGIN_NAMESPACE

class QUrl;
class QSize;
class QAudioFormat;
class QCamera;
class QCameraDevice;
class QMediaFormat;
class QAudioDevice;
class QMediaCaptureSession;
class QPlatformMediaRecorder;

class QMediaRecorderPrivate;
class Q_MULTIMEDIA_EXPORT QMediaRecorder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QMediaRecorder::RecorderState recorderState READ recorderState NOTIFY recorderStateChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(QUrl outputLocation READ outputLocation WRITE setOutputLocation)
    Q_PROPERTY(QUrl actualLocation READ actualLocation NOTIFY actualLocationChanged)
    Q_PROPERTY(QMediaMetaData metaData READ metaData WRITE setMetaData NOTIFY metaDataChanged)
    Q_PROPERTY(QMediaRecorder::Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    Q_PROPERTY(QMediaFormat mediaFormat READ mediaFormat WRITE setMediaFormat NOTIFY mediaFormatChanged)
    Q_PROPERTY(Quality quality READ quality WRITE setQuality NOTIFY qualityChanged)
    Q_PROPERTY(QMediaRecorder::EncodingMode encodingMode READ encodingMode WRITE setEncodingMode NOTIFY encodingModeChanged)
    Q_PROPERTY(QSize videoResolution READ videoResolution WRITE setVideoResolution NOTIFY videoResolutionChanged)
    Q_PROPERTY(qreal videoFrameRate READ videoFrameRate WRITE setVideoFrameRate NOTIFY videoFrameRateChanged)
    Q_PROPERTY(int videoBitRate READ videoBitRate WRITE setVideoBitRate NOTIFY videoBitRateChanged)
    Q_PROPERTY(int audioBitRate READ audioBitRate WRITE setAudioBitRate NOTIFY audioBitRateChanged)
    Q_PROPERTY(int audioChannelCount READ audioChannelCount WRITE setAudioChannelCount NOTIFY audioChannelCountChanged)
    Q_PROPERTY(int audioSampleRate READ audioSampleRate WRITE setAudioSampleRate NOTIFY audioSampleRateChanged)
public:
    enum Quality
    {
        VeryLowQuality,
        LowQuality,
        NormalQuality,
        HighQuality,
        VeryHighQuality
    };
    Q_ENUM(Quality)

    enum EncodingMode
    {
        ConstantQualityEncoding,
        ConstantBitRateEncoding,
        AverageBitRateEncoding,
        TwoPassEncoding
    };
    Q_ENUM(EncodingMode)

    enum RecorderState
    {
        StoppedState,
        RecordingState,
        PausedState
    };
    Q_ENUM(RecorderState)

    enum Error
    {
        NoError,
        ResourceError,
        FormatError,
        OutOfSpaceError,
        LocationNotWritable
    };
    Q_ENUM(Error)

    QMediaRecorder(QObject *parent = nullptr);
    ~QMediaRecorder();

    bool isAvailable() const;

    QUrl outputLocation() const;
    void setOutputLocation(const QUrl &location);

    QUrl actualLocation() const;

    RecorderState recorderState() const;

    Error error() const;
    QString errorString() const;

    qint64 duration() const;

    QMediaFormat mediaFormat() const;
    void setMediaFormat(const QMediaFormat &format);

    EncodingMode encodingMode() const;
    void setEncodingMode(EncodingMode);

    Quality quality() const;
    void setQuality(Quality quality);

    QSize videoResolution() const;
    void setVideoResolution(const QSize &);
    void setVideoResolution(int width, int height) { setVideoResolution(QSize(width, height)); }

    qreal videoFrameRate() const;
    void setVideoFrameRate(qreal frameRate);

    int videoBitRate() const;
    void setVideoBitRate(int bitRate);

    int audioBitRate() const;
    void setAudioBitRate(int bitRate);

    int audioChannelCount() const;
    void setAudioChannelCount(int channels);

    int audioSampleRate() const;
    void setAudioSampleRate(int sampleRate);

    QMediaMetaData metaData() const;
    void setMetaData(const QMediaMetaData &metaData);
    void addMetaData(const QMediaMetaData &metaData);

    QMediaCaptureSession *captureSession() const;
    QPlatformMediaRecorder *platformRecoder() const;

public Q_SLOTS:
    void record();
    void pause();
    void stop();

Q_SIGNALS:
    void recorderStateChanged(RecorderState state);
    void durationChanged(qint64 duration);
    void actualLocationChanged(const QUrl &location);
    void encoderSettingsChanged();

    void errorOccurred(Error error, const QString &errorString);
    void errorChanged();

    void metaDataChanged();

    void mediaFormatChanged();
    void encodingModeChanged();
    void qualityChanged();
    void videoResolutionChanged();
    void videoFrameRateChanged();
    void videoBitRateChanged();
    void audioBitRateChanged();
    void audioChannelCountChanged();
    void audioSampleRateChanged();

private:
    QMediaRecorderPrivate *d_ptr;
    friend class QMediaCaptureSession;
    void setCaptureSession(QMediaCaptureSession *session);
    Q_DISABLE_COPY(QMediaRecorder)
    Q_DECLARE_PRIVATE(QMediaRecorder)
};

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QMediaRecorder, RecorderState)
Q_MEDIA_ENUM_DEBUG(QMediaRecorder, Error)

#endif  // QMediaRecorder_H

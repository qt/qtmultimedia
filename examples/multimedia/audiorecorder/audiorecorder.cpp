// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "audiorecorder.h"
#include "audiolevel.h"
#include "ui_audiorecorder.h"

#include <QAudioBuffer>
#include <QAudioDevice>
#include <QAudioInput>
#include <QDir>
#include <QFileDialog>
#include <QImageCapture>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QMimeType>
#include <QStandardPaths>

#if QT_CONFIG(permissions)
  #include <QPermission>
#endif

static QList<qreal> getBufferLevels(const QAudioBuffer &buffer);

AudioRecorder::AudioRecorder() : ui(new Ui::AudioRecorder)
{
    ui->setupUi(this);

    // channels
    ui->channelsBox->addItem(tr("Default"), QVariant(-1));
    ui->channelsBox->addItem(QStringLiteral("1"), QVariant(1));
    ui->channelsBox->addItem(QStringLiteral("2"), QVariant(2));
    ui->channelsBox->addItem(QStringLiteral("4"), QVariant(4));

    // quality
    ui->qualitySlider->setRange(0, int(QImageCapture::VeryHighQuality));
    ui->qualitySlider->setValue(int(QImageCapture::NormalQuality));

    // bit rates:
    ui->bitrateBox->addItem(tr("Default"), QVariant(0));
    ui->bitrateBox->addItem(QStringLiteral("32000"), QVariant(32000));
    ui->bitrateBox->addItem(QStringLiteral("64000"), QVariant(64000));
    ui->bitrateBox->addItem(QStringLiteral("96000"), QVariant(96000));
    ui->bitrateBox->addItem(QStringLiteral("128000"), QVariant(128000));

    // audio input initialization
    init();
}

void AudioRecorder::init()
{
#if QT_CONFIG(permissions)
    QMicrophonePermission microphonePermission;
    switch (qApp->checkPermission(microphonePermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(microphonePermission, this, &AudioRecorder::init);
        return;
    case Qt::PermissionStatus::Denied:
        qWarning("Microphone permission is not granted!");
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif

    m_audioRecorder = new QMediaRecorder(this);
    m_captureSession.setRecorder(m_audioRecorder);
    m_captureSession.setAudioInput(new QAudioInput(this));
    // ### replace with a monitoring output once we have it.
    // m_probe = new QAudioProbe(this);
    // connect(m_probe, &QAudioProbe::audioBufferProbed,
    //         this, &AudioRecorder::processBuffer);
    // m_probe->setSource(m_audioRecorder);

    // audio devices
    m_mediaDevices = new QMediaDevices(this);
    connect(m_mediaDevices, &QMediaDevices::audioInputsChanged, this,
            &AudioRecorder::updateDevices);
    updateDevices();

    // audio codecs and container formats
    updateFormats();
    connect(ui->audioCodecBox, &QComboBox::currentIndexChanged, this,
            &AudioRecorder::updateFormats);
    connect(ui->containerBox, &QComboBox::currentIndexChanged, this, &AudioRecorder::updateFormats);

    // sample rate
    constexpr auto allSamplingRates = std::array{
        8000,  11025, 12000, 16000, 22050,  24000,  32000,  44100,
        48000, 64000, 88200, 96000, 128000, 176400, 192000,
    };

    QAudioDevice device = m_captureSession.audioInput()->device();
    int minSamplingRate = device.minimumSampleRate();
    int maxSamplingRate = device.maximumSampleRate();

    for (int rate : allSamplingRates) {
        if (rate < minSamplingRate || rate > maxSamplingRate)
            continue;
        ui->sampleRateBox->addItem(QString::number(rate), rate);
    }
    int preferredRate = device.preferredFormat().sampleRate();
    if (preferredRate > 0) {
        int index = ui->sampleRateBox->findData(device.preferredFormat().sampleRate());
        ui->sampleRateBox->setCurrentIndex(index);
    }

    connect(m_audioRecorder, &QMediaRecorder::durationChanged, this,
            &AudioRecorder::updateProgress);
    connect(m_audioRecorder, &QMediaRecorder::recorderStateChanged, this,
            &AudioRecorder::onStateChanged);
    connect(m_audioRecorder, &QMediaRecorder::errorChanged, this,
            &AudioRecorder::displayErrorMessage);
    connect(m_audioRecorder, &QMediaRecorder::mediaFormatChanged, this,
            &AudioRecorder::onMediaFormatChanged);
}

void AudioRecorder::updateProgress(qint64 duration)
{
    if (m_audioRecorder->error() != QMediaRecorder::NoError || duration < 2000)
        return;

    ui->statusbar->showMessage(tr("Recorded %1 sec").arg(duration / 1000));
}

void AudioRecorder::onStateChanged(QMediaRecorder::RecorderState state)
{
    QString statusMessage;

    switch (state) {
    case QMediaRecorder::RecordingState:
        statusMessage = tr("Recording to %1").arg(m_audioRecorder->actualLocation().toString());
        ui->recordButton->setText(tr("Stop"));
        ui->pauseButton->setText(tr("Pause"));
        break;
    case QMediaRecorder::PausedState:
        clearAudioLevels();
        statusMessage = tr("Paused");
        ui->recordButton->setText(tr("Stop"));
        ui->pauseButton->setText(tr("Resume"));
        break;
    case QMediaRecorder::StoppedState:
        clearAudioLevels();
        statusMessage = tr("Stopped");
        ui->recordButton->setText(tr("Record"));
        ui->pauseButton->setText(tr("Pause"));
        break;
    }

    ui->pauseButton->setEnabled(m_audioRecorder->recorderState() != QMediaRecorder::StoppedState);
    if (m_audioRecorder->error() == QMediaRecorder::NoError)
        ui->statusbar->showMessage(statusMessage);
}

static QVariant boxValue(const QComboBox *box)
{
    int idx = box->currentIndex();
    if (idx == -1)
        return QVariant();

    return box->itemData(idx);
}

void AudioRecorder::toggleRecord()
{
    if (!m_audioRecorder) {
        ui->statusbar->showMessage(QStringLiteral("AudioRecorder not set. "
                                                  "Microphone permission was not granted!"));
        return;
    }

    if (m_audioRecorder->recorderState() == QMediaRecorder::StoppedState) {
        m_captureSession.audioInput()->setDevice(
                boxValue(ui->audioDeviceBox).value<QAudioDevice>());

        m_audioRecorder->setMediaFormat(selectedMediaFormat());
        m_audioRecorder->setAudioSampleRate(boxValue(ui->sampleRateBox).toInt());
        m_audioRecorder->setAudioBitRate(boxValue(ui->bitrateBox).toInt());
        m_audioRecorder->setAudioChannelCount(boxValue(ui->channelsBox).toInt());
        m_audioRecorder->setQuality(QMediaRecorder::Quality(ui->qualitySlider->value()));
        m_audioRecorder->setEncodingMode(ui->constantQualityRadioButton->isChecked()
                                                 ? QMediaRecorder::ConstantQualityEncoding
                                                 : QMediaRecorder::ConstantBitRateEncoding);

        m_audioRecorder->record();
    } else {
        m_audioRecorder->stop();
    }
}

void AudioRecorder::togglePause()
{
    if (m_audioRecorder->recorderState() != QMediaRecorder::PausedState)
        m_audioRecorder->pause();
    else
        m_audioRecorder->record();
}

void AudioRecorder::setOutputLocation()
{
    if (!m_audioRecorder) {
        ui->statusbar->showMessage(QStringLiteral("AudioRecorder not set. "
                                                  "Microphone permission was not granted!"));
        return;
    }

#ifdef Q_OS_ANDROID
    QMediaFormat format = selectedMediaFormat();
    if (format.fileFormat() == QMediaFormat::UnspecifiedFormat)
        format.resolveForEncoding(QMediaFormat::NoFlags);

    QString fileName = QFileDialog::getSaveFileName(
            this, tr("Save Recording"),
            "output." + format.mimeType().preferredSuffix());
#else
    QString fileName = QFileDialog::getSaveFileName();
#endif
    m_audioRecorder->setOutputLocation(QUrl::fromLocalFile(fileName));
    m_outputLocationSet = true;
}

void AudioRecorder::displayErrorMessage()
{
    ui->statusbar->showMessage(m_audioRecorder->errorString());
}

void AudioRecorder::onMediaFormatChanged()
{
    QMediaFormat format = m_audioRecorder->mediaFormat();

    int formatIndex = ui->containerBox->findData(format.fileFormat());
    if (formatIndex != -1)
        ui->containerBox->setCurrentIndex(formatIndex);

    int codecIndex = ui->audioCodecBox->findData(QVariant::fromValue(format.audioCodec()));
    if (codecIndex != -1)
        ui->audioCodecBox->setCurrentIndex(codecIndex);
}

void AudioRecorder::updateDevices()
{
    const auto currentDevice = boxValue(ui->audioDeviceBox).value<QAudioDevice>();
    int currentDeviceIndex = 0;

    ui->audioDeviceBox->clear();

    ui->audioDeviceBox->addItem(tr("Default"), {});
    for (const auto &device : m_mediaDevices->audioInputs()) {
        const auto name = device.description();
        ui->audioDeviceBox->addItem(name, QVariant::fromValue(device));

        if (device.id() == currentDevice.id())
            currentDeviceIndex = ui->audioDeviceBox->count() - 1;
    }

    ui->audioDeviceBox->setCurrentIndex(currentDeviceIndex);
}

void AudioRecorder::updateFormats()
{
    if (m_updatingFormats)
        return;
    m_updatingFormats = true;

    QMediaFormat format;
    if (ui->containerBox->count())
        format.setFileFormat(boxValue(ui->containerBox).value<QMediaFormat::FileFormat>());

    // Add all supported formats, remembering the previous choice
    int currentIndex = 0;
    ui->containerBox->clear();
    ui->containerBox->addItem(tr("Default file format"),
                              QVariant::fromValue(QMediaFormat::UnspecifiedFormat));
    for (auto container : format.supportedFileFormats(QMediaFormat::Encode)) {
        if (container < QMediaFormat::Mpeg4Audio) // Skip video formats
            continue;
        if (container == format.fileFormat())
            currentIndex = ui->containerBox->count();
        ui->containerBox->addItem(QMediaFormat::fileFormatDescription(container),
                                  QVariant::fromValue(container));
    }
    ui->containerBox->setCurrentIndex(currentIndex);

    // Re-populate codecs, adding only those that the chosen file format supports.
    // Remember current codec if it is supported by the file format.
    QMediaFormat::AudioCodec currentCodec = QMediaFormat::AudioCodec::Unspecified;
    if (ui->audioCodecBox->count())
        currentCodec = boxValue(ui->audioCodecBox).value<QMediaFormat::AudioCodec>();

    currentIndex = 0;
    ui->audioCodecBox->clear();
    ui->audioCodecBox->addItem(tr("Default audio codec"),
                               QVariant::fromValue(QMediaFormat::AudioCodec::Unspecified));
    for (auto codec : format.supportedAudioCodecs(QMediaFormat::Encode)) {
        if (codec == currentCodec)
            currentIndex = ui->audioCodecBox->count();
        ui->audioCodecBox->addItem(QMediaFormat::audioCodecDescription(codec),
                                   QVariant::fromValue(codec));
    }
    ui->audioCodecBox->setCurrentIndex(currentIndex);

    m_updatingFormats = false;
}

void AudioRecorder::clearAudioLevels()
{
    for (auto m_audioLevel : std::as_const(m_audioLevels))
        m_audioLevel->setLevel(0);
}

QMediaFormat AudioRecorder::selectedMediaFormat() const
{
    QMediaFormat format;
    format.setFileFormat(boxValue(ui->containerBox).value<QMediaFormat::FileFormat>());
    format.setAudioCodec(boxValue(ui->audioCodecBox).value<QMediaFormat::AudioCodec>());
    return format;
}

// returns the audio level for each channel
QList<qreal> getBufferLevels(const QAudioBuffer &buffer)
{
    QList<qreal> values;

    auto format = buffer.format();
    if (!format.isValid())
        return values;

    int channels = buffer.format().channelCount();
    values.fill(0, channels);

    int bytesPerSample = format.bytesPerSample();
    QList<qreal> max_values;
    max_values.fill(0, channels);

    const char *data = buffer.constData<char>();
    for (int i = 0; i < buffer.frameCount(); ++i) {
        for (int j = 0; j < channels; ++j) {
            qreal value = qAbs(format.normalizedSampleValue(data));
            if (value > max_values.at(j))
                max_values[j] = value;
            data += bytesPerSample;
        }
    }

    return max_values;
}

void AudioRecorder::processBuffer(const QAudioBuffer &buffer)
{
    if (m_audioLevels.count() != buffer.format().channelCount()) {
        qDeleteAll(m_audioLevels);
        m_audioLevels.clear();
        for (int i = 0; i < buffer.format().channelCount(); ++i) {
            AudioLevel *level = new AudioLevel(ui->centralwidget);
            m_audioLevels.append(level);
            ui->levelsLayout->addWidget(level);
        }
    }

    QList<qreal> levels = getBufferLevels(buffer);
    for (int i = 0; i < levels.count(); ++i)
        m_audioLevels.at(i)->setLevel(levels.at(i));
}


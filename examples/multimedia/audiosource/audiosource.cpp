// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "audiosource.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QtEndian>

#if QT_CONFIG(permissions)
#  include <QCoreApplication>
#  include <QPermission>
#endif

namespace {

using namespace std::chrono_literals;
constexpr auto visualizerUpdateInterval = 16ms;
constexpr auto recordingUpdateInterval = 100ms;
constexpr int volumeSliderMaximum = 100;
constexpr int progressBarMaximum = 100;

float calculateLevel(const char *data, qint64 len, const QAudioFormat &format)
{
    const int channelBytes = format.bytesPerSample();
    const int sampleBytes = format.bytesPerFrame();
    Q_ASSERT(format.bytesPerFrame() != 0); // divide by 0
    const quint64 numSamples = len / sampleBytes;

    float maxValue = 0;
    const auto *ptr = reinterpret_cast<const unsigned char *>(data);

    for (int i = 0; i < int(numSamples); ++i) {
        for (int j = 0; j < format.channelCount(); ++j) {
            float value = format.normalizedSampleValue(ptr);

            maxValue = qMax(value, maxValue);
            ptr += channelBytes;
        }
    }
    return maxValue;
}

using namespace Qt::Literals::StringLiterals;
QString sampleFormatToString(QAudioFormat::SampleFormat f)
{
    switch (f) {
    case QAudioFormat::UInt8: return u"UInt8"_s;
    case QAudioFormat::Int16: return u"Int16"_s;
    case QAudioFormat::Int32: return u"Int32"_s;
    case QAudioFormat::Float: return u"Float"_s;
    default:                  return u"Unknown"_s;
    }
}

QString audioModeToString(AudioTestMode mode)
{
    switch (mode) {
    case AudioTestMode::Pull:       return u"Pull Mode"_s;
    case AudioTestMode::Push:       return u"Push Mode"_s;
    case AudioTestMode::Callback:   return u"Callback Mode"_s;
    default:                        return u"Unknown"_s;
    }
}

constexpr std::array allSupportedSampleRates{
    8'000,  11'025, 12'000, 16'000, 22'050,  24'000,  32'000,  44'100,
    48'000, 64'000, 88'200, 96'000, 128'000, 176'400, 192'000,
};

template <typename T>
void setCurrentValue(QComboBox *box, const T &value)
{
    int idx = box->findData(QVariant::fromValue(value));
    if (idx >= 0)
        box->setCurrentIndex(idx);
}

void syncFormatGui(QComboBox *m_formatBox, QComboBox *m_channelsBox, QComboBox *m_rateBox,
                   const QAudioFormat &format)
{
    setCurrentValue(m_formatBox, format.sampleFormat());
    setCurrentValue(m_rateBox, format.sampleRate());
    setCurrentValue(m_channelsBox, format.channelCount());
}

} // namespace

AudioInfo::AudioInfo(const QAudioFormat &format) : m_format(format) { }

void AudioInfo::start()
{
    open(QIODevice::WriteOnly);
}

void AudioInfo::stop()
{
    close();
}

qint64 AudioInfo::readData(char * /* data */, qint64 /* maxlen */)
{
    return 0;
}

qint64 AudioInfo::writeData(const char *data, qint64 len)
{
    m_level = calculateLevel(data, len, m_format);

    emit levelChanged(m_level);

    return len;
}

RenderArea::RenderArea(QWidget *parent) : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    setMinimumHeight(30);
    setMinimumWidth(200);
}

void RenderArea::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);

    painter.setPen(Qt::black);

    const QRect frame = painter.viewport() - QMargins(10, 10, 10, 10);
    painter.drawRect(frame);
    if (m_level == 0.0)
        return;

    float remappedLevel = QtAudio::convertVolume(m_level, QtAudio::LinearVolumeScale,
                                                 QtAudio::LogarithmicVolumeScale);

    const int pos = qRound(qreal(frame.width() - 1) * remappedLevel);
    painter.fillRect(frame.left() + 1, frame.top() + 1, pos, frame.height() - 1, Qt::red);
}

void RenderArea::setLevel(float value)
{
    m_level = value;
    update();
}

InputTest::InputTest()
    : m_devices(new QMediaDevices(this))
{
    init();
}

InputTest::~InputTest()
{
    cleanupAudioSource();
}

void InputTest::initializeWindow()
{
    m_layout = new QVBoxLayout(this);

    m_canvas = new RenderArea(this);
    m_layout->addWidget(m_canvas);

    m_deviceBox = new QComboBox(this);
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioInput();
    for (auto &deviceInfo : QMediaDevices::audioInputs())
        m_deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    auto defaultDeviceIndex = m_deviceBox->findData(QVariant::fromValue(defaultDevice));
    m_deviceBox->setCurrentIndex(defaultDeviceIndex);

    connect(m_deviceBox, &QComboBox::currentIndexChanged, this, &InputTest::deviceChanged);
    connect(m_devices, &QMediaDevices::audioInputsChanged, this, &InputTest::updateAudioDevices);
    m_layout->addWidget(m_deviceBox);

    m_modeBox = new QComboBox(this);
    m_modeBox->addItem(audioModeToString(AudioTestMode::Pull));
    m_modeBox->addItem(audioModeToString(AudioTestMode::Push));
    m_modeBox->addItem(audioModeToString(AudioTestMode::Callback));
    m_modeBox->setCurrentIndex(qToUnderlying(m_mode));
    connect(m_modeBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_mode = AudioTestMode(index);
        restartAudioStream(false);
    });
    m_layout->addWidget(m_modeBox);

    m_suspendResumeButton = new QPushButton(this);
    connect(m_suspendResumeButton, &QPushButton::clicked, this, &InputTest::toggleSuspend);
    m_layout->addWidget(m_suspendResumeButton);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, volumeSliderMaximum);
    m_volumeSlider->setValue(volumeSliderMaximum);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &InputTest::sliderChanged);
    m_layout->addWidget(m_volumeSlider);

    auto *formatBox = new QHBoxLayout;

    // Sample format selector
    auto *formatLabel = new QLabel(tr("Sample Format:"));
    m_formatBox = new QComboBox(this);

    // Sample rate selector
    auto *rateLabel = new QLabel(tr("Sample Rate:"));
    m_rateBox = new QComboBox(this);

    // Channel count selector
    auto *chLabel = new QLabel(tr("Channels:"));
    m_channelsBox = new QComboBox(this);

    for (auto *box : { m_channelsBox, m_rateBox, m_formatBox })
        connect(box, &QComboBox::activated, this, [this, box]() {
            formatChanged(box);
        });

    // add all to the same row
    const int horizontalSpacing = 12;
    formatBox->addWidget(formatLabel);
    formatBox->addWidget(m_formatBox);
    formatBox->addSpacing(horizontalSpacing);
    formatBox->addWidget(rateLabel);
    formatBox->addWidget(m_rateBox);
    formatBox->addSpacing(horizontalSpacing);
    formatBox->addWidget(chLabel);
    formatBox->addWidget(m_channelsBox);

    m_layout->addLayout(formatBox);

    // Native period frame count selector
    QHBoxLayout *periodLayout = new QHBoxLayout;
    auto *periodLabel = new QLabel(tr("Period Size:"));
    m_periodBox = new QComboBox(this);
    m_periodBox->addItem(tr("Default"), -1);
    for (int v = 32; v <= 4096; v *= 2)
        m_periodBox->addItem(QString::number(v), v);
    connect(m_periodBox, &QComboBox::activated, this, [this]() {
        restartAudioStream(false);
    });
    periodLayout->addWidget(periodLabel);
    periodLayout->addWidget(m_periodBox);
    m_layout->addLayout(periodLayout);

    m_recordButton = new QPushButton(tr("Start 5s Recording"), this);
    connect(m_recordButton, &QPushButton::clicked, this, [this]() {
        if (m_audioSource && !m_currentDevice.isNull())
            restartAudioStream(true);
    });
    m_layout->addWidget(m_recordButton);

    m_recordProgressBar = new QProgressBar(this);
    m_recordProgressBar->hide();
    m_recordProgressBar->setRange(0, progressBarMaximum);
    m_recordProgressBar->setFormat(tr("Recording... %p%"));
    m_layout->addWidget(m_recordProgressBar);
}

void InputTest::startAudioSource(const QAudioDevice &device, const QAudioFormat &format)
{
    if (m_audioSource)
        cleanupAudioSource();

    m_audioSource = std::make_unique<QAudioSource>(device, format);
    m_audioSource->setNativePeriodFrameCount(m_periodBox->currentData().toInt());

    m_currentDevice = device;

    syncFormatGui(m_formatBox, m_channelsBox, m_rateBox, m_audioSource->format());

    connect(m_audioSource.get(), &QAudioSource::stateChanged, this,
            [this, device](QAudio::State state) {
        switch (state) {
        case QAudio::ActiveState:
            m_suspendResumeButton->setText(tr("Suspend capture"));
            return;
        case QAudio::SuspendedState:
            m_suspendResumeButton->setText(tr("Resume capture"));
            return;
        default:
            break;
        }

        const auto err = m_audioSource->error();

        // startup failure (format rejected or device unavailable)
        if (err == QAudio::OpenError && state == QAudio::StoppedState) {
            QMessageBox::warning(this, tr("Audio start failed"),
                                    tr("Device rejected the format or is unavailable."));
            return;
        }

        // runtime I/O or fatal device error (disconnects, etc.)
        if (err == QAudio::IOError || err == QAudio::FatalError) {
            if (m_currentDevice == device) {
                m_currentDevice = {};
                m_deviceBox->setCurrentIndex(-1);
            }

            QMessageBox::warning(this, tr("Audio error"), tr("Audio device error."));

            return;
        }
    });

    m_audioInfo = std::make_unique<AudioInfo>(format);
    connect(m_audioInfo.get(), &AudioInfo::levelChanged, m_canvas, &RenderArea::setLevel);

    float initialVolume =
            QAudio::convertVolume(float(m_audioSource->volume()), QAudio::LinearVolumeScale,
                                  QAudio::LogarithmicVolumeScale);
    m_volumeSlider->setValue(qRound(initialVolume * volumeSliderMaximum));

    m_audioInfo->start();
    restartAudioStream(false);
}

void InputTest::cleanupAudioSource()
{
    if (m_audioInfo)
        m_audioInfo->stop();

    if (m_audioSource) {
        m_audioSource->stop();
        m_audioSource->disconnect(this);
    }

    m_audioSource.reset();
    m_currentDevice = {};
}

void InputTest::initializeErrorWindow()
{
    auto *layout = new QVBoxLayout(this);
    auto *errorLabel = new QLabel(tr("Microphone permission is not granted!"));
    errorLabel->setWordWrap(true);
    errorLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(errorLabel);
}

void InputTest::restartAudioStream(bool record)
{
    m_audioSource->stop();
    m_audioSource->setNativePeriodFrameCount(m_periodBox->currentData().toInt());
    setRecorder(nullptr); // Stops possible recording

    if (m_callbackVisualizerTimer.isActive())
        m_callbackVisualizerTimer.stop();

    switch (m_mode) {
    case AudioTestMode::Pull: {
        startPullMode(record);
        break;
    }
    case AudioTestMode::Push: {
        startPushMode(record);
        break;
    }
    case AudioTestMode::Callback: {
        startCallbackMode(record);
        break;
    }
    default:
        Q_UNREACHABLE();
    }

    if (m_audioSource->error() != QAudio::NoError) {
        QMessageBox::warning(this, tr("Audio start failed"),
                             tr("Device rejected the format or is unavailable."));
    }
}

void InputTest::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_callbackVisualizerTimer.timerId())
        m_canvas->setLevel(m_level.exchange(0.f));
    else if (event->timerId() == m_recordingProgressTimer.timerId()) {
        Q_ASSERT(m_recorder);
        if (m_recorder->isDone())
            restartAudioStream(false);
        else
            updateRecordingProgress();
    }
}

template <typename T>
void InputTest::processCallback(QSpan<const T> buffer, const QAudioFormat &format)
{
    auto *callbackRecorder = static_cast<CallbackRecorder *>(m_recorder.get());
    if (callbackRecorder)
        callbackRecorder->writeSpan(as_bytes(buffer));

    float level = calculateLevel(reinterpret_cast<const char *>(buffer.data()), buffer.size_bytes(),
                                 format);
    float lastLevel = m_level.load(std::memory_order_relaxed);
    while (!m_level.compare_exchange_weak(lastLevel, std::max(level, lastLevel)))
        ;
}

void InputTest::startPullMode(bool record)
{
    // pull mode: QAudioSource provides a QIODevice to pull from
    auto *readDevice = m_audioSource->start();
    if (!readDevice)
        return;

    if (record)
        setRecorder(std::make_unique<PullRecorder>(m_audioSource->format(),
                                                   getRecordingFileName()));

    connect(readDevice, &QIODevice::readyRead, this, [this, readDevice]() {
        const qint64 maxLength = m_audioSource->bytesAvailable();
        QByteArray buffer(maxLength, 0);
        const qint64 bytesRead = readDevice->read(buffer.data(), maxLength);
        if (bytesRead <= 0)
            return;

        const float level = calculateLevel(buffer.constData(), bytesRead, m_audioSource->format());
        m_canvas->setLevel(level);

        auto *pullRecorder = static_cast<PullRecorder *>(m_recorder.get());
        if (!pullRecorder)
            return;

        pullRecorder->writeSpan(
                { reinterpret_cast<const std::byte *>(buffer.constData()), (qsizetype)bytesRead });
    });
}

void InputTest::startPushMode(bool record)
{
    // push mode: QAudioSource pushes data into QIODevice
    if (record) {
        auto pushRecorder = std::make_unique<PushRecorder>(m_audioSource->format(), getRecordingFileName());
        pushRecorder->open(QIODeviceBase::WriteOnly);
        m_audioSource->start(pushRecorder.get());
        setRecorder(std::move(pushRecorder));
    } else {
        m_audioSource->start(m_audioInfo.get());
    }
}

void InputTest::startCallbackMode(bool record)
{
    // callback mode: QAudioSource calls a callback function on audio thread with a buffer to read from
    if (record)
        setRecorder(std::make_unique<CallbackRecorder>(m_audioSource->format(),
                                                    getRecordingFileName()));

    QAudioFormat format = m_audioSource->format();
    switch (format.sampleFormat()) {
    case QAudioFormat::UInt8:
        m_audioSource->start([this, format](QSpan<const uint8_t> buffer) {
            processCallback(buffer, format);
        });
        break;
    case QAudioFormat::Int16:
        m_audioSource->start([this, format](QSpan<const int16_t> buffer) {
            processCallback(buffer, format);
        });
        break;
    case QAudioFormat::Int32:
        m_audioSource->start([this, format](QSpan<const int32_t> buffer) {
            processCallback(buffer, format);
        });
        break;
    case QAudioFormat::Float:
        m_audioSource->start([this, format](QSpan<const float> buffer) {
            processCallback(buffer, format);
        });
        break;
    default:
        Q_UNREACHABLE();
    };

    m_callbackVisualizerTimer.start(visualizerUpdateInterval, Qt::PreciseTimer, this);
}

QString InputTest::getRecordingFileName() const
{
    QDir tempDir(QDir::tempPath());
    return tempDir.filePath(
            QStringLiteral("%1-%2-%3.wav")
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"))
                    .arg(audioModeToString(m_mode))
                    .arg(m_deviceBox->currentText().split(u" "_s).at(0))
                    .replace(u" "_s, u"_"_s));
}

void InputTest::init()
{
#if QT_CONFIG(permissions)
    QMicrophonePermission microphonePermission;
    switch (qApp->checkPermission(microphonePermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(microphonePermission, this, &InputTest::init);
        return;
    case Qt::PermissionStatus::Denied:
        qWarning("Microphone permission is not granted!");
        initializeErrorWindow();
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif
    initializeWindow();

    deviceChanged(m_deviceBox->currentIndex());
}

void InputTest::toggleSuspend()
{
    // toggle suspend/resume
    switch (m_audioSource->state()) {
    case QAudio::SuspendedState:
        m_audioSource->resume();
        break;
    case QAudio::ActiveState:
        m_audioSource->suspend();
        break;
    default:
        // no-op
        break;
    }
}

void InputTest::deviceChanged(int index)
{
    auto device = m_deviceBox->itemData(index).value<QAudioDevice>();

    // clear format selectors
    m_formatBox->clear();
    m_channelsBox->clear();
    m_rateBox->clear();

    // Populate format selectors
    if (!device.isNull()) {
        // sample formats
        const auto formats = device.supportedSampleFormats();
        for (const QAudioFormat::SampleFormat sf : formats)
            m_formatBox->addItem(sampleFormatToString(sf), QVariant::fromValue(sf));

        // channels
        for (int ch = device.minimumChannelCount(); ch <= device.maximumChannelCount(); ++ch)
            m_channelsBox->addItem(QString::number(ch), ch);

        // populate from the hardcoded list
        for (int rate : allSupportedSampleRates) {
            if (rate < device.minimumSampleRate() || rate > device.maximumSampleRate())
                continue;
            m_rateBox->addItem(QString::number(rate), rate);
        }
    }

    if (device != m_currentDevice) {
        cleanupAudioSource();
        if (!device.isNull()) {
            startAudioSource(m_deviceBox->itemData(index).value<QAudioDevice>(),
                             device.preferredFormat());
        }
    }
}

void InputTest::sliderChanged(int value)
{
    float linearVolume =
            QAudio::convertVolume(float(value) / float(volumeSliderMaximum),
                                  QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale);

    m_audioSource->setVolume(linearVolume);
}

void InputTest::updateAudioDevices()
{
    QSignalBlocker blockUpdates(m_deviceBox);

    m_deviceBox->clear();

    const QList<QAudioDevice> devices = QMediaDevices::audioInputs();
    for (const QAudioDevice &deviceInfo : devices)
        m_deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    const int currentDeviceIndex = m_deviceBox->findData(QVariant::fromValue(m_currentDevice));
    if (currentDeviceIndex != -1) {
        // select previous device
        m_deviceBox->setCurrentIndex(currentDeviceIndex);
    } else {
        blockUpdates.unblock();
        // select default device
        QAudioDevice defaultDevice = QMediaDevices::defaultAudioInput();
        const int defaultDeviceIndex = m_deviceBox->findData(QVariant::fromValue(defaultDevice));
        const int currentIndex = m_deviceBox->currentIndex();
        m_deviceBox->setCurrentIndex(defaultDeviceIndex);
        if (defaultDeviceIndex == currentIndex) {
            // device changed, reinitialize audio
            deviceChanged(defaultDeviceIndex);
        }
    }
}

void InputTest::formatChanged(QComboBox *box)
{
    auto device = m_deviceBox->currentData().value<QAudioDevice>();
    QAudioFormat newFormat = m_audioSource->format();

    if (box == m_formatBox)
        newFormat.setSampleFormat(QAudioFormat::SampleFormat(box->currentData().toInt()));
    else if (box == m_rateBox)
        newFormat.setSampleRate(box->currentData().toInt());
    else if (box == m_channelsBox)
        newFormat.setChannelCount(box->currentData().toInt());

    startAudioSource(device, newFormat);
}

void InputTest::updateRecordingProgress()
{
    if (!m_recorder) {
        // Recording completed
        m_recordingProgressTimer.stop();
    } else {
        m_recordProgressBar->setValue(m_recorder->progress(progressBarMaximum));

        if (!m_recordingProgressTimer.isActive()) {
            // Recording started
            m_recordingProgressTimer.start(recordingUpdateInterval, Qt::CoarseTimer, this);
        }
    }
}

void InputTest::updateControlsForRecording()
{
    bool isRecording = !!m_recorder;

    if (isRecording) {
        m_recordButton->hide();
        m_recordProgressBar->show();
    } else {
        m_recordProgressBar->hide();
        m_recordButton->show();
    }

    m_channelsBox->setDisabled(isRecording);
    m_deviceBox->setDisabled(isRecording);
    m_formatBox->setDisabled(isRecording);
    m_modeBox->setDisabled(isRecording);
    m_rateBox->setDisabled(isRecording);
}

void InputTest::setRecorder(std::unique_ptr<AudioRecorder> recorder)
{
    m_recorder = std::move(recorder);
    updateRecordingProgress();
    updateControlsForRecording();
}

#include "moc_audiosource.cpp"

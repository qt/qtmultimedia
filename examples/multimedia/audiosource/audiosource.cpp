// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "audiosource.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QDateTime>
#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QVBoxLayout>
#include <QtEndian>

#if QT_CONFIG(permissions)
#  include <QCoreApplication>
#  include <QPermission>
#endif

#include <math.h>
#include <stdlib.h>

namespace {

using namespace std::chrono_literals;
constexpr auto visualizerUpdateInterval = 16ms;

float calculateLevel(const char *data, qint64 len,const QAudioFormat &format)
{
    const int channelBytes = format.bytesPerSample();
    const int sampleBytes = format.bytesPerFrame();
    Q_ASSERT(format.bytesPerFrame() != 0); // divide by 0
    const int numSamples = len / sampleBytes;

    float maxValue = 0;
    auto *ptr = reinterpret_cast<const unsigned char *>(data);

    for (int i = 0; i < numSamples; ++i) {
        for (int j = 0; j < format.channelCount(); ++j) {
            float value = format.normalizedSampleValue(ptr);

            maxValue = qMax(value, maxValue);
            ptr += channelBytes;
        }
    }
    return maxValue;
}

}

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

void RenderArea::setLevel(qreal value)
{
    m_level = value;
    update();
}

InputTest::InputTest() : m_devices(new QMediaDevices(this))
{
    init();
}

void InputTest::initializeWindow()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_canvas = new RenderArea(this);
    layout->addWidget(m_canvas);

    m_deviceBox = new QComboBox(this);
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioInput();
    for (auto &deviceInfo : QMediaDevices::audioInputs())
        m_deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    auto defaultDeviceIndex = m_deviceBox->findData(QVariant::fromValue(defaultDevice));
    m_deviceBox->setCurrentIndex(defaultDeviceIndex);

    connect(m_deviceBox, &QComboBox::activated, this, &InputTest::deviceChanged);
    connect(m_devices, &QMediaDevices::audioInputsChanged, this, &InputTest::updateAudioDevices);
    layout->addWidget(m_deviceBox);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &InputTest::sliderChanged);
    layout->addWidget(m_volumeSlider);

    m_modeBox = new QComboBox(this);
    m_modeBox->addItem(tr("Pull Mode"));
    m_modeBox->addItem(tr("Push Mode"));
    m_modeBox->addItem(tr("Callback Mode"));
    m_modeBox->setCurrentIndex(qToUnderlying(m_mode));
    connect(m_modeBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_mode = static_cast<AudioTestMode>(index);
        restartAudioStream();
    });
    layout->addWidget(m_modeBox);

    m_suspendResumeButton = new QPushButton(this);
    connect(m_suspendResumeButton, &QPushButton::clicked, this, &InputTest::toggleSuspend);
    layout->addWidget(m_suspendResumeButton);
}

void InputTest::startAudioSource(const QAudioDevice &device)
{
    if (m_audioSource)
        cleanupAudioSource();

    m_audioSource = std::make_unique<QAudioSource>(device, device.preferredFormat());

    m_currentDevice = device;

    connect(m_audioSource.get(), &QAudioSource::stateChanged, this,
            [this, device](QAudio::State state) {
        switch (state) {
        case QAudio::ActiveState:
            m_suspendResumeButton->setText(tr("Suspend playback"));
            return;
        case QAudio::SuspendedState:
            m_suspendResumeButton->setText(tr("Resume playback"));
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

    QAudioFormat format = device.preferredFormat();
    m_audioInfo = std::make_unique<AudioInfo>(format);
    connect(m_audioInfo.get(), &AudioInfo::levelChanged, m_canvas, &RenderArea::setLevel);

    qreal initialVolume = QAudio::convertVolume(m_audioSource->volume(), QAudio::LinearVolumeScale,
                                                QAudio::LogarithmicVolumeScale);
    m_volumeSlider->setValue(qRound(initialVolume * 100));

    m_audioInfo->start();
    restartAudioStream();
}

void InputTest::cleanupAudioSource()
{
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
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *errorLabel = new QLabel(tr("Microphone permission is not granted!"));
    errorLabel->setWordWrap(true);
    errorLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(errorLabel);
}

void InputTest::restartAudioStream()
{
    m_audioSource->stop();

    if (m_callbackVisualizerTimer.isActive())
        m_callbackVisualizerTimer.stop();

    switch (m_mode) {
    case AudioTestMode::Pull: {
        // pull mode: QAudioSource provides a QIODevice to pull from
        auto *io = m_audioSource->start();
        if (!io)
            return;

        connect(io, &QIODevice::readyRead, this, [this, io]() {
            static const qint64 BufferSize = 4096;
            const qint64 len = qMin(m_audioSource->bytesAvailable(), BufferSize);

            QByteArray buffer(len, 0);
            qint64 l = io->read(buffer.data(), len);
            if (l > 0) {
                const qreal level = calculateLevel(buffer.constData(), l, m_audioSource->format());
                m_canvas->setLevel(level);
            }
        });
        break;
    }
    case AudioTestMode::Push: {
        // push mode: QIODevice pushes data into QIODevice
        m_audioSource->start(m_audioInfo.get());
        break;
    }
    case AudioTestMode::Callback: {
        // callback mode: QAudioSource calls a callback function on audio thread with a buffer to read from
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
            m_audioSource->start( [this, format](QSpan<const float> buffer) {
                processCallback(buffer, format);
            });
            break;
        default:
            Q_UNREACHABLE();
        };

        m_callbackVisualizerTimer.start(visualizerUpdateInterval, Qt::PreciseTimer, this);
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
}

template <typename T>
void InputTest::processCallback(QSpan<const T> buffer, const QAudioFormat &format)
{
    float level = calculateLevel(reinterpret_cast<const char *>(buffer.data()), buffer.size_bytes(),
                                 format);
    float lastLevel = m_level.load(std::memory_order_relaxed);
    while (!m_level.compare_exchange_weak(lastLevel, std::max(level, lastLevel)))
        ;
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
    startAudioSource(QMediaDevices::defaultAudioInput());
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
    QAudioDevice dev = m_deviceBox->itemData(index).value<QAudioDevice>();

    if (dev != m_currentDevice) {
        cleanupAudioSource();
        if (!dev.isNull()) {
            startAudioSource(m_deviceBox->itemData(index).value<QAudioDevice>());
        }
    }
}

void InputTest::sliderChanged(int value)
{
    qreal linearVolume = QAudio::convertVolume(value / qreal(100), QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);

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

#include "moc_audiosource.cpp"

// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "audiosource.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QDateTime>
#include <QDebug>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QtEndian>

#if QT_CONFIG(permissions)
  #include <QCoreApplication>
  #include <QPermission>
#endif

#include <math.h>
#include <stdlib.h>

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

qreal AudioInfo::calculateLevel(const char *data, qint64 len) const
{
    const int channelBytes = m_format.bytesPerSample();
    const int sampleBytes = m_format.bytesPerFrame();
    const int numSamples = len / sampleBytes;

    float maxValue = 0;
    auto *ptr = reinterpret_cast<const unsigned char *>(data);

    for (int i = 0; i < numSamples; ++i) {
        for (int j = 0; j < m_format.channelCount(); ++j) {
            float value = m_format.normalizedSampleValue(ptr);

            maxValue = qMax(value, maxValue);
            ptr += channelBytes;
        }
    }
    return maxValue;
}

qint64 AudioInfo::writeData(const char *data, qint64 len)
{
    m_level = calculateLevel(data, len);

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
    connect(m_deviceBox, &QComboBox::activated, this, &InputTest::deviceChanged);
    connect(m_devices, &QMediaDevices::audioInputsChanged, this, &InputTest::updateAudioDevices);
    updateAudioDevices();
    layout->addWidget(m_deviceBox);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &InputTest::sliderChanged);
    layout->addWidget(m_volumeSlider);

    m_modeButton = new QPushButton(this);
    connect(m_modeButton, &QPushButton::clicked, this, &InputTest::toggleMode);
    layout->addWidget(m_modeButton);

    m_suspendResumeButton = new QPushButton(this);
    connect(m_suspendResumeButton, &QPushButton::clicked, this, &InputTest::toggleSuspend);
    layout->addWidget(m_suspendResumeButton);

    connect(this, &InputTest::pullModeChanged, this, [&] {
        if (m_pullMode)
            m_modeButton->setText(tr("Enable push mode"));
        else
            m_modeButton->setText(tr("Enable pull mode"));
    });
    emit pullModeChanged();
}

void InputTest::initializeAudio(const QAudioDevice &deviceInfo)
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    bool sampleRateSupported = format.sampleRate() < deviceInfo.maximumSampleRate()
            && format.sampleRate() > deviceInfo.minimumSampleRate();
    bool channelCountSupported = format.channelCount() < deviceInfo.maximumChannelCount()
            && format.channelCount() > deviceInfo.minimumChannelCount();

    if (!sampleRateSupported)
        format.setSampleRate(deviceInfo.preferredFormat().sampleRate());

    if (!channelCountSupported)
        format.setChannelCount(deviceInfo.preferredFormat().channelCount());

    m_audioInfo.reset(new AudioInfo(format));
    connect(m_audioInfo.get(), &AudioInfo::levelChanged, m_canvas, &RenderArea::setLevel);

    m_audioSource.reset(new QAudioSource(deviceInfo, format));
    qreal initialVolume = QAudio::convertVolume(m_audioSource->volume(), QAudio::LinearVolumeScale,
                                                QAudio::LogarithmicVolumeScale);
    m_volumeSlider->setValue(qRound(initialVolume * 100));
    m_audioInfo->start();
    restartAudioStream();

    m_suspendResumeButton->setText(tr("Suspend playback"));
    connect(m_audioSource.get(), &QAudioSource::stateChanged, this, [this](QAudio::State state) {
        switch (state) {
        case QAudio::SuspendedState:
            m_suspendResumeButton->setText(tr("Resume playback"));
            break;
        default:
            m_suspendResumeButton->setText(tr("Suspend playback"));
            break;
        }
    });
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

    if (m_pullMode) {
        // pull mode: QAudioSource provides a QIODevice to pull from
        auto *io = m_audioSource->start();
        if (!io)
            return;

        connect(io, &QIODevice::readyRead, [this, io]() {
            static const qint64 BufferSize = 4096;
            const qint64 len = qMin(m_audioSource->bytesAvailable(), BufferSize);

            QByteArray buffer(len, 0);
            qint64 l = io->read(buffer.data(), len);
            if (l > 0) {
                const qreal level = m_audioInfo->calculateLevel(buffer.constData(), l);
                m_canvas->setLevel(level);
            }
        });
    } else {
        // push mode: QIODevice pushes data into QIODevice
        m_audioSource->start(m_audioInfo.get());
    }
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
    initializeAudio(QMediaDevices::defaultAudioInput());
}

void InputTest::toggleMode()
{
    m_pullMode = !m_pullMode;
    emit pullModeChanged();

    restartAudioStream();
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
    m_audioSource->stop();
    m_audioSource->disconnect(this);
    m_audioInfo->stop();

    initializeAudio(m_deviceBox->itemData(index).value<QAudioDevice>());
}

void InputTest::sliderChanged(int value)
{
    qreal linearVolume = QAudio::convertVolume(value / qreal(100), QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);

    m_audioSource->setVolume(linearVolume);
}

void InputTest::updateAudioDevices()
{
    m_deviceBox->clear();
    const QAudioDevice &defaultDeviceInfo = QMediaDevices::defaultAudioInput();
    m_deviceBox->addItem(defaultDeviceInfo.description(), QVariant::fromValue(defaultDeviceInfo));
    for (auto &deviceInfo : m_devices->audioInputs()) {
        if (deviceInfo != defaultDeviceInfo)
            m_deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    }
}

#include "moc_audiosource.cpp"

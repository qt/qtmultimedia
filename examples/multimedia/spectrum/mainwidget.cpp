// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwidget.h"
#include "engine.h"
#include "levelmeter.h"
#include "progressbar.h"
#include "settingsdialog.h"
#include "spectrograph.h"
#include "tonegeneratordialog.h"
#include "utils.h"
#include "waveform.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QTimerEvent>
#include <QVBoxLayout>

const int NullTimerId = -1;

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent),
      m_mode(NoMode),
      m_engine(new Engine(this))
#ifndef DISABLE_WAVEFORM
      ,
      m_waveform(new Waveform(this))
#endif
      ,
      m_progressBar(new ProgressBar(this)),
      m_spectrograph(new Spectrograph(this)),
      m_levelMeter(new LevelMeter(this)),
      m_modeButton(new QPushButton(this)),
      m_recordButton(new QPushButton(this)),
      m_pauseButton(new QPushButton(this)),
      m_playButton(new QPushButton(this)),
      m_settingsButton(new QPushButton(this)),
      m_infoMessage(new QLabel(tr("Select a mode to begin"), this)),
      m_infoMessageTimerId(NullTimerId),
      m_settingsDialog(new SettingsDialog(m_engine->availableAudioInputDevices(),
                                          m_engine->availableAudioOutputDevices(), this)),
      m_toneGeneratorDialog(new ToneGeneratorDialog(this)),
      m_modeMenu(new QMenu(this)),
      m_loadFileAction(nullptr),
      m_generateToneAction(nullptr),
      m_recordAction(nullptr),
      m_errorOccurred(false)
{
    m_spectrograph->setParams(SpectrumNumBands, SpectrumLowFreq, SpectrumHighFreq);

    createUi();
    connectUi();
}

MainWidget::~MainWidget() = default;

//-----------------------------------------------------------------------------
// Public slots
//-----------------------------------------------------------------------------

void MainWidget::stateChanged(QAudioDevice::Mode mode, QAudio::State state)
{
    Q_UNUSED(mode);

    updateButtonStates();

    if (QAudio::ActiveState != state && QAudio::SuspendedState != state) {
        m_levelMeter->reset();
        m_spectrograph->reset();
    }
}

void MainWidget::formatChanged(const QAudioFormat &format)
{
    infoMessage(formatToString(format), NullMessageTimeout);

#ifndef DISABLE_WAVEFORM
    if (QAudioFormat() != format) {
        m_waveform->initialize(format, WaveformTileLength, WaveformWindowDuration);
    }
#endif
}

void MainWidget::spectrumChanged(qint64 position, qint64 length, const FrequencySpectrum &spectrum)
{
    m_progressBar->windowChanged(position, length);
    m_spectrograph->spectrumChanged(spectrum);
}

void MainWidget::infoMessage(const QString &message, int timeoutMs)
{
    m_infoMessage->setText(message);

    if (NullTimerId != m_infoMessageTimerId) {
        killTimer(m_infoMessageTimerId);
        m_infoMessageTimerId = NullTimerId;
    }

    if (NullMessageTimeout != timeoutMs)
        m_infoMessageTimerId = startTimer(timeoutMs);
}

void MainWidget::errorMessage(const QString &heading, const QString &detail)
{
    QMessageBox::warning(this, heading, detail, QMessageBox::Close);
    m_errorOccurred = true;
    reset();
}

void MainWidget::timerEvent(QTimerEvent *event)
{
    Q_ASSERT(event->timerId() == m_infoMessageTimerId);
    Q_UNUSED(event); // suppress warnings in release builds
    killTimer(m_infoMessageTimerId);
    m_infoMessageTimerId = NullTimerId;
    m_infoMessage->setText("");
}

void MainWidget::audioPositionChanged(qint64 position)
{
#ifndef DISABLE_WAVEFORM
    m_waveform->audioPositionChanged(position);
#else
    Q_UNUSED(position);
#endif
}

void MainWidget::bufferLengthChanged(qint64 length)
{
    m_progressBar->bufferLengthChanged(length);
}

//-----------------------------------------------------------------------------
// Private slots
//-----------------------------------------------------------------------------

void MainWidget::showFileDialog()
{
    m_errorOccurred = false;
    const QString dir;
    const QStringList fileNames =
            QFileDialog::getOpenFileNames(this, tr("Open WAV file"), dir, "*.wav");
    if (fileNames.count()) {
        reset();
        setMode(LoadFileMode);
        m_engine->loadFile(fileNames.front());
        updateButtonStates();
    } else {
        updateModeMenu();
    }
}

void MainWidget::showSettingsDialog()
{
    m_settingsDialog->exec();
    if (m_settingsDialog->result() == QDialog::Accepted) {
        m_engine->setAudioInputDevice(m_settingsDialog->inputDevice());
        m_engine->setAudioOutputDevice(m_settingsDialog->outputDevice());
        m_engine->setWindowFunction(m_settingsDialog->windowFunction());
    }
}

void MainWidget::showToneGeneratorDialog()
{
    m_errorOccurred = false;
    m_toneGeneratorDialog->exec();
    if (m_toneGeneratorDialog->result() == QDialog::Accepted) {
        reset();
        setMode(GenerateToneMode);
        const qreal amplitude = m_toneGeneratorDialog->amplitude();
        if (m_toneGeneratorDialog->isFrequencySweepEnabled()) {
            m_engine->generateSweptTone(amplitude);
        } else {
            const qreal frequency = m_toneGeneratorDialog->frequency();
            const Tone tone(frequency, amplitude);
            m_engine->generateTone(tone);
            updateButtonStates();
        }
    } else {
        setMode(NoMode);
        updateModeMenu();
    }
}

void MainWidget::initializeRecord()
{
    m_errorOccurred = false;
    reset();
    setMode(RecordMode);
    if (m_engine->initializeRecord())
        updateButtonStates();
}

//-----------------------------------------------------------------------------
// Private functions
//-----------------------------------------------------------------------------

void MainWidget::createUi()
{
    createMenus();

    setWindowTitle(tr("Spectrum Analyser"));

    QVBoxLayout *windowLayout = new QVBoxLayout(this);

    m_infoMessage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_infoMessage->setAlignment(Qt::AlignHCenter);
    windowLayout->addWidget(m_infoMessage);

#ifdef SUPERIMPOSE_PROGRESS_ON_WAVEFORM
    std::unique_ptr<QHBoxLayout> waveformLayout(new QHBoxLayout);
    waveformLayout->addWidget(m_progressBar);
    m_progressBar->setMinimumHeight(m_waveform->minimumHeight());
    waveformLayout->setContentsMargins(0, 0, 0, 0);
    m_waveform->setLayout(waveformLayout.release());
    windowLayout->addWidget(m_waveform);
#else
#    ifndef DISABLE_WAVEFORM
    windowLayout->addWidget(m_waveform);
#    endif // DISABLE_WAVEFORM
    windowLayout->addWidget(m_progressBar);
#endif // SUPERIMPOSE_PROGRESS_ON_WAVEFORM

    // Spectrograph and level meter

    std::unique_ptr<QHBoxLayout> analysisLayout(new QHBoxLayout);
    analysisLayout->addWidget(m_spectrograph);
    analysisLayout->addWidget(m_levelMeter);
    windowLayout->addLayout(analysisLayout.release());

    // Button panel

    const QSize buttonSize(30, 30);

    m_modeButton->setText(tr("Mode"));

    m_recordIcon = QIcon(":/images/record.png");
    m_recordButton->setIcon(m_recordIcon);
    m_recordButton->setEnabled(false);
    m_recordButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_recordButton->setMinimumSize(buttonSize);

    m_pauseIcon = style()->standardIcon(QStyle::SP_MediaPause);
    m_pauseButton->setIcon(m_pauseIcon);
    m_pauseButton->setEnabled(false);
    m_pauseButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pauseButton->setMinimumSize(buttonSize);

    m_playIcon = style()->standardIcon(QStyle::SP_MediaPlay);
    m_playButton->setIcon(m_playIcon);
    m_playButton->setEnabled(false);
    m_playButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_playButton->setMinimumSize(buttonSize);

    m_settingsIcon = QIcon(":/images/settings.png");
    m_settingsButton->setIcon(m_settingsIcon);
    m_settingsButton->setEnabled(true);
    m_settingsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_settingsButton->setMinimumSize(buttonSize);

    std::unique_ptr<QHBoxLayout> buttonPanelLayout(new QHBoxLayout);
    buttonPanelLayout->addStretch();
    buttonPanelLayout->addWidget(m_modeButton);
    buttonPanelLayout->addWidget(m_recordButton);
    buttonPanelLayout->addWidget(m_pauseButton);
    buttonPanelLayout->addWidget(m_playButton);
    buttonPanelLayout->addWidget(m_settingsButton);

    QWidget *buttonPanel = new QWidget(this);
    buttonPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    buttonPanel->setLayout(buttonPanelLayout.release());

    std::unique_ptr<QHBoxLayout> bottomPaneLayout(new QHBoxLayout);
    bottomPaneLayout->addWidget(buttonPanel);
    windowLayout->addLayout(bottomPaneLayout.release());

    // Apply layout

    setLayout(windowLayout);
}

void MainWidget::connectUi()
{
    connect(m_recordButton, &QPushButton::clicked, m_engine, &Engine::startRecording);

    connect(m_pauseButton, &QPushButton::clicked, m_engine, &Engine::suspend);

    connect(m_playButton, &QPushButton::clicked, m_engine, &Engine::startPlayback);

    connect(m_settingsButton, &QPushButton::clicked, this, &MainWidget::showSettingsDialog);

    connect(m_engine, &Engine::stateChanged, this, &MainWidget::stateChanged);

    connect(m_engine, &Engine::formatChanged, this, &MainWidget::formatChanged);

    m_progressBar->bufferLengthChanged(m_engine->bufferLength());

    connect(m_engine, &Engine::bufferLengthChanged, this, &MainWidget::bufferLengthChanged);

    connect(m_engine, &Engine::dataLengthChanged, this, &MainWidget::updateButtonStates);

    connect(m_engine, &Engine::recordPositionChanged, m_progressBar,
            &ProgressBar::recordPositionChanged);

    connect(m_engine, &Engine::playPositionChanged, m_progressBar,
            &ProgressBar::playPositionChanged);

    connect(m_engine, &Engine::recordPositionChanged, this, &MainWidget::audioPositionChanged);

    connect(m_engine, &Engine::playPositionChanged, this, &MainWidget::audioPositionChanged);

    connect(m_engine, &Engine::levelChanged, m_levelMeter, &LevelMeter::levelChanged);

    connect(m_engine,
            QOverload<qint64, qint64, const FrequencySpectrum &>::of(&Engine::spectrumChanged),
            this,
            QOverload<qint64, qint64, const FrequencySpectrum &>::of(&MainWidget::spectrumChanged));

    connect(m_engine, &Engine::infoMessage, this, &MainWidget::infoMessage);

    connect(m_engine, &Engine::errorMessage, this, &MainWidget::errorMessage);

    connect(m_spectrograph, &Spectrograph::infoMessage, this, &MainWidget::infoMessage);

#ifndef DISABLE_WAVEFORM
    connect(m_engine, &Engine::bufferChanged, m_waveform, &Waveform::bufferChanged);
#endif
}

void MainWidget::createMenus()
{
    m_modeButton->setMenu(m_modeMenu);

    m_generateToneAction = m_modeMenu->addAction(tr("Play generated tone"));
    m_recordAction = m_modeMenu->addAction(tr("Record and play back"));
    m_loadFileAction = m_modeMenu->addAction(tr("Play file"));

    m_loadFileAction->setCheckable(true);
    m_generateToneAction->setCheckable(true);
    m_recordAction->setCheckable(true);

    connect(m_loadFileAction, &QAction::triggered, this, &MainWidget::showFileDialog);
    connect(m_generateToneAction, &QAction::triggered, this, &MainWidget::showToneGeneratorDialog);
    connect(m_recordAction, &QAction::triggered, this, &MainWidget::initializeRecord);
}

void MainWidget::updateButtonStates()
{
    const bool recordEnabled = ((QAudioDevice::Output == m_engine->mode()
                                 || (QAudio::ActiveState != m_engine->state()
                                     && QAudio::IdleState != m_engine->state()))
                                && RecordMode == m_mode);
    m_recordButton->setEnabled(m_errorOccurred ? false : recordEnabled);

    const bool pauseEnabled =
            (QAudio::ActiveState == m_engine->state() || QAudio::IdleState == m_engine->state());
    m_pauseButton->setEnabled(m_errorOccurred ? false : pauseEnabled);

    const bool playEnabled = (/*m_engine->dataLength() &&*/
                              (QAudioDevice::Output != m_engine->mode()
                               || (QAudio::ActiveState != m_engine->state()
                                   && QAudio::IdleState != m_engine->state())));

    m_playButton->setEnabled(m_errorOccurred ? false : playEnabled);
}

void MainWidget::reset()
{
#ifndef DISABLE_WAVEFORM
    m_waveform->reset();
#endif
    m_engine->reset();
    m_levelMeter->reset();
    m_spectrograph->reset();
    m_progressBar->reset();
    if (m_errorOccurred) {
        setMode(Mode::NoMode);
        updateButtonStates();
    }
}

void MainWidget::setMode(Mode mode)
{
    m_mode = mode;
    updateModeMenu();
}

void MainWidget::updateModeMenu()
{
    m_loadFileAction->setChecked(LoadFileMode == m_mode);
    m_generateToneAction->setChecked(GenerateToneMode == m_mode);
    m_recordAction->setChecked(RecordMode == m_mode);
}

#include "moc_mainwidget.cpp"

// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "player.h"
#include "playercontrols.h"
#include "playlistmodel.h"
#include "qmediaplaylist.h"
#include "videowidget.h"
#include "audiolevelmeter.h"

#include <QApplication>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QBoxLayout>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QMediaMetaData>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QStandardPaths>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QAudioBufferOutput>

using namespace Qt::Literals;

Player::Player(QWidget *parent) : QWidget(parent)
{
    //! [create-objs]
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    //! [create-objs]
    connect(m_player, &QMediaPlayer::durationChanged, this, &Player::durationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &Player::positionChanged);
    connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this,
            &Player::metaDataChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &Player::statusChanged);
    connect(m_player, &QMediaPlayer::bufferProgressChanged, this, &Player::bufferingProgress);
    connect(m_player, &QMediaPlayer::hasVideoChanged, this, &Player::videoAvailableChanged);
    connect(m_player, &QMediaPlayer::errorChanged, this, &Player::displayErrorMessage);
    connect(m_player, &QMediaPlayer::tracksChanged, this, &Player::tracksChanged);

    //! [2]
    m_videoWidget = new VideoWidget(this);
    m_videoWidget->resize(1280, 720);
    m_player->setVideoOutput(m_videoWidget);

    m_playlistModel = new PlaylistModel(this);
    m_playlist = m_playlistModel->playlist();
    //! [2]
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this,
            &Player::playlistPositionChanged);

    // audio level meter
    m_audioBufferOutput = new QAudioBufferOutput(this);
    m_player->setAudioBufferOutput(m_audioBufferOutput);
    m_audioLevelMeter = new AudioLevelMeter(this);
    connect(m_audioBufferOutput, &QAudioBufferOutput::audioBufferReceived,
            m_audioLevelMeter, &AudioLevelMeter::onAudioBufferReceived);
    connect(m_player, &QMediaPlayer::playingChanged,
            m_audioLevelMeter, &AudioLevelMeter::deactivate);

    // player layout
    QBoxLayout *layout = new QVBoxLayout(this);

    // display
    QBoxLayout *displayLayout = new QHBoxLayout;
    displayLayout->addWidget(m_videoWidget, 2);
    displayLayout->addWidget(m_audioLevelMeter, 3);
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    m_playlistView = new QListView();
    m_playlistView->setModel(m_playlistModel);
    m_playlistView->setCurrentIndex(m_playlistModel->index(m_playlist->currentIndex(), 0));
    connect(m_playlistView, &QAbstractItemView::activated, this, &Player::jump);
    displayLayout->addWidget(m_playlistView);
#endif
    layout->addLayout(displayLayout);

    // duration slider and label
    QHBoxLayout *hLayout = new QHBoxLayout;

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(0, m_player->duration());
    connect(m_slider, &QSlider::sliderMoved, this, &Player::seek);
    hLayout->addWidget(m_slider);

    m_labelDuration = new QLabel();
    m_labelDuration->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    hLayout->addWidget(m_labelDuration);
    layout->addLayout(hLayout);

    // controls
    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton *openButton = new QPushButton(tr("Open"), this);
    connect(openButton, &QPushButton::clicked, this, &Player::open);
    controlLayout->addWidget(openButton);
    controlLayout->addStretch(1);

    PlayerControls *controls = new PlayerControls();
    controls->setState(m_player->playbackState());
    controls->setVolume(m_audioOutput->volume());
    controls->setMuted(controls->isMuted());

    connect(controls, &PlayerControls::play, m_player, &QMediaPlayer::play);
    connect(controls, &PlayerControls::pause, m_player, &QMediaPlayer::pause);
    connect(controls, &PlayerControls::stop, m_player, &QMediaPlayer::stop);
    connect(controls, &PlayerControls::next, m_playlist, &QMediaPlaylist::next);
    connect(controls, &PlayerControls::previous, this, &Player::previousClicked);
    connect(controls, &PlayerControls::changeVolume, m_audioOutput, &QAudioOutput::setVolume);
    connect(controls, &PlayerControls::changeMuting, m_audioOutput, &QAudioOutput::setMuted);
    connect(controls, &PlayerControls::changeRate, m_player, &QMediaPlayer::setPlaybackRate);
    connect(controls, &PlayerControls::stop, m_videoWidget, QOverload<>::of(&QVideoWidget::update));

    connect(m_player, &QMediaPlayer::playbackStateChanged, controls,
            [controls](QMediaPlayer::PlaybackState arg) {
        controls->setState(arg);
    });
    connect(m_audioOutput, &QAudioOutput::volumeChanged, controls, &PlayerControls::setVolume);
    connect(m_audioOutput, &QAudioOutput::mutedChanged, controls, &PlayerControls::setMuted);

    controlLayout->addWidget(controls);
    controlLayout->addStretch(1);

    m_fullScreenButton = new QPushButton(tr("FullScreen"), this);
    m_fullScreenButton->setCheckable(true);
    controlLayout->addWidget(m_fullScreenButton);

    m_pitchCompensationButton = new QPushButton(tr("Pitch compensation"), this);
    m_pitchCompensationButton->setCheckable(true);
    switch (m_player->pitchCompensationAvailability()) {
    case QMediaPlayer::PitchCompensationAvailability::AlwaysOn:
        m_pitchCompensationButton->setEnabled(false);
        m_pitchCompensationButton->setChecked(true);
        m_pitchCompensationButton->setToolTip(
                u"Pitch compensation always enabled on this backend"_s);
        break;
    case QMediaPlayer::PitchCompensationAvailability::Unavailable:
        m_pitchCompensationButton->setEnabled(false);
        m_pitchCompensationButton->setChecked(false);
        m_pitchCompensationButton->setToolTip(u"Pitch compensation unavailable on this backend"_s);
        break;
    case QMediaPlayer::PitchCompensationAvailability::Available:
        m_pitchCompensationButton->setEnabled(true);
        m_pitchCompensationButton->setChecked(m_player->pitchCompensation());
        break;
    default:
        Q_UNREACHABLE();
    }

    controlLayout->addWidget(m_pitchCompensationButton);
    connect(m_player, &QMediaPlayer::pitchCompensationChanged, this, [this] {
        m_pitchCompensationButton->setChecked(m_player->pitchCompensation());
    });
    m_pitchCompensationButton->setChecked(m_player->pitchCompensation());
    connect(m_pitchCompensationButton, &QPushButton::toggled, m_player,
            &QMediaPlayer::setPitchCompensation);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    m_audioOutputCombo = new QComboBox(this);
    controlLayout->addWidget(m_audioOutputCombo);

    updateAudioDevices();

    connect(m_audioOutputCombo, QOverload<int>::of(&QComboBox::activated), this,
            &Player::audioOutputChanged);

    QObject::connect(&m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, [this] {
        updateAudioDevices();
    });
#endif

    layout->addLayout(controlLayout);

    // tracks
    QGridLayout *tracksLayout = new QGridLayout;

    m_audioTracks = new QComboBox(this);
    connect(m_audioTracks, &QComboBox::activated, this, &Player::selectAudioStream);
    tracksLayout->addWidget(new QLabel(tr("Audio Tracks:")), 0, 0);
    tracksLayout->addWidget(m_audioTracks, 0, 1);

    m_videoTracks = new QComboBox(this);
    connect(m_videoTracks, &QComboBox::activated, this, &Player::selectVideoStream);
    tracksLayout->addWidget(new QLabel(tr("Video Tracks:")), 1, 0);
    tracksLayout->addWidget(m_videoTracks, 1, 1);

    m_subtitleTracks = new QComboBox(this);
    connect(m_subtitleTracks, &QComboBox::activated, this, &Player::selectSubtitleStream);
    tracksLayout->addWidget(new QLabel(tr("Subtitle Tracks:")), 2, 0);
    tracksLayout->addWidget(m_subtitleTracks, 2, 1);

    layout->addLayout(tracksLayout);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    // metadata

    QLabel *metaDataLabel = new QLabel(tr("Metadata for file:"));
    layout->addWidget(metaDataLabel);

    QGridLayout *metaDataLayout = new QGridLayout;
    int key = QMediaMetaData::Title;
    for (int i = 0; i < (QMediaMetaData::NumMetaData + 2) / 3; i++) {
        for (int j = 0; j < 6; j += 2) {
            m_metaDataLabels[key] = new QLabel(
                    QMediaMetaData::metaDataKeyToString(static_cast<QMediaMetaData::Key>(key)));
            if (key == QMediaMetaData::ThumbnailImage || key == QMediaMetaData::CoverArtImage) {
                m_metaDataFields[key] = new QLabel;
            } else {
                auto lineEdit = new QLineEdit;
                lineEdit->setReadOnly(true);
                m_metaDataFields[key] = lineEdit;
            }
            m_metaDataLabels[key]->setDisabled(true);
            m_metaDataFields[key]->setDisabled(true);
            metaDataLayout->addWidget(m_metaDataLabels[key], i, j);
            metaDataLayout->addWidget(m_metaDataFields[key], i, j + 1);
            key++;
            if (key == QMediaMetaData::NumMetaData)
                break;
        }
    }

    layout->addLayout(metaDataLayout);
#endif

#if defined(Q_OS_QNX)
    // On QNX, the main window doesn't have a title bar (or any other decorations).
    // Create a status bar for the status information instead.
    m_statusLabel = new QLabel;
    m_statusBar = new QStatusBar;
    m_statusBar->addPermanentWidget(m_statusLabel);
    m_statusBar->setSizeGripEnabled(false); // Without mouse grabbing, it doesn't work very well.
    layout->addWidget(m_statusBar);
#endif

    setLayout(layout);

    if (!isPlayerAvailable()) {
        QMessageBox::warning(this, tr("Service not available"),
                             tr("The QMediaPlayer object does not have a valid service.\n"
                                "Please check the media service plugins are installed."));

        controls->setEnabled(false);
        if (m_playlistView)
            m_playlistView->setEnabled(false);
        openButton->setEnabled(false);
        m_fullScreenButton->setEnabled(false);
    }

    metaDataChanged();
}

bool Player::isPlayerAvailable() const
{
    return m_player->isAvailable();
}

void Player::open()
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open Files"));
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                                    .value(0, QDir::homePath()));
    if (fileDialog.exec() == QDialog::Accepted)
        addToPlaylist(fileDialog.selectedUrls());
}

static bool isPlaylist(const QUrl &url) // Check for ".m3u" playlists.
{
    if (!url.isLocalFile())
        return false;
    const QFileInfo fileInfo(url.toLocalFile());
    return fileInfo.exists()
            && !fileInfo.suffix().compare(QLatin1String("m3u"), Qt::CaseInsensitive);
}

void Player::addToPlaylist(const QList<QUrl> &urls)
{
    const int previousMediaCount = m_playlist->mediaCount();
    for (auto &url : urls) {
        if (isPlaylist(url))
            m_playlist->load(url);
        else
            m_playlist->addMedia(url);
    }
    if (m_playlist->mediaCount() > previousMediaCount) {
        auto index = m_playlistModel->index(previousMediaCount, 0);
        if (m_playlistView)
            m_playlistView->setCurrentIndex(index);
        jump(index);
    }
}

void Player::durationChanged(qint64 duration)
{
    m_duration = duration / 1000;
    m_slider->setMaximum(duration);
}

void Player::positionChanged(qint64 progress)
{
    if (!m_slider->isSliderDown())
        m_slider->setValue(progress);

    updateDurationInfo(progress / 1000);
}

void Player::metaDataChanged()
{
    auto metaData = m_player->metaData();
    setTrackInfo(QStringLiteral("%1 - %2")
                         .arg(metaData.value(QMediaMetaData::AlbumArtist).toString())
                         .arg(metaData.value(QMediaMetaData::Title).toString()));

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    for (int i = 0; i < QMediaMetaData::NumMetaData; i++) {
        if (QLineEdit *field = qobject_cast<QLineEdit *>(m_metaDataFields[i]))
            field->clear();
        else if (QLabel *label = qobject_cast<QLabel *>(m_metaDataFields[i]))
            label->clear();
        m_metaDataFields[i]->setDisabled(true);
        m_metaDataLabels[i]->setDisabled(true);
    }

    for (auto &&[key, value] : metaData.asKeyValueRange()) {
        int i = int(key);
        if (key == QMediaMetaData::CoverArtImage) {
            if (QLabel *cover = qobject_cast<QLabel *>(m_metaDataFields[key])) {
                QImage coverImage = value.value<QImage>();
                cover->setPixmap(QPixmap::fromImage(coverImage));
            }
        } else if (key == QMediaMetaData::ThumbnailImage) {
            if (QLabel *thumbnail = qobject_cast<QLabel *>(m_metaDataFields[key])) {
                QImage thumbnailImage = value.value<QImage>();
                thumbnail->setPixmap(QPixmap::fromImage(thumbnailImage));
            }
        } else if (QLineEdit *field = qobject_cast<QLineEdit *>(m_metaDataFields[key])) {
            QString stringValue = metaData.stringValue(key);
            field->setText(stringValue);
        }
        m_metaDataFields[i]->setDisabled(false);
        m_metaDataLabels[i]->setDisabled(false);
    }

    const QList<QMediaMetaData> tracks = m_player->videoTracks();
    const int currentVideoTrack = m_player->activeVideoTrack();
    if (currentVideoTrack >= 0 && currentVideoTrack < tracks.size()) {
        const QMediaMetaData track = tracks.value(currentVideoTrack);
        for (const QMediaMetaData::Key &key : track.keys()) {
            if (QLineEdit *field = qobject_cast<QLineEdit *>(m_metaDataFields[key])) {
                QString stringValue = track.stringValue(key);
                field->setText(stringValue);
            }
            m_metaDataFields[key]->setDisabled(true);
            m_metaDataLabels[key]->setDisabled(true);
        }
    }
#endif
}

QString Player::trackName(const QMediaMetaData &metaData, int index)
{
    QString name;
    QString title = metaData.stringValue(QMediaMetaData::Title);
    QLocale::Language lang = metaData.value(QMediaMetaData::Language).value<QLocale::Language>();

    if (title.isEmpty()) {
        if (lang == QLocale::Language::AnyLanguage)
            name = tr("Track %1").arg(index + 1);
        else
            name = QLocale::languageToString(lang);
    } else {
        if (lang == QLocale::Language::AnyLanguage)
            name = title;
        else
            name = QStringLiteral("%1 - [%2]").arg(title).arg(QLocale::languageToString(lang));
    }
    return name;
}

void Player::tracksChanged()
{
    m_audioTracks->clear();
    m_videoTracks->clear();
    m_subtitleTracks->clear();

    const auto audioTracks = m_player->audioTracks();
    m_audioTracks->addItem(QStringLiteral("No audio"), -1);
    for (int i = 0; i < audioTracks.size(); ++i)
        m_audioTracks->addItem(trackName(audioTracks.at(i), i), i);
    m_audioTracks->setCurrentIndex(m_player->activeAudioTrack() + 1);

    const auto videoTracks = m_player->videoTracks();
    m_videoTracks->addItem(QStringLiteral("No video"), -1);
    for (int i = 0; i < videoTracks.size(); ++i)
        m_videoTracks->addItem(trackName(videoTracks.at(i), i), i);
    m_videoTracks->setCurrentIndex(m_player->activeVideoTrack() + 1);

    m_subtitleTracks->addItem(QStringLiteral("No subtitles"), -1);
    const auto subtitleTracks = m_player->subtitleTracks();
    for (int i = 0; i < subtitleTracks.size(); ++i)
        m_subtitleTracks->addItem(trackName(subtitleTracks.at(i), i), i);
    m_subtitleTracks->setCurrentIndex(m_player->activeSubtitleTrack() + 1);
}

void Player::previousClicked()
{
    // Go to previous track if we are within the first 5 seconds of playback
    // Otherwise, seek to the beginning.
    if (m_player->position() <= 5000) {
        m_playlist->previous();
    } else {
        m_player->setPosition(0);
    }
}

void Player::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        m_playlist->setCurrentIndex(index.row());
    }
}

void Player::playlistPositionChanged(int currentItem)
{
    if (m_playlistView)
        m_playlistView->setCurrentIndex(m_playlistModel->index(currentItem, 0));
    m_player->setSource(m_playlist->currentMedia());
}

void Player::seek(int mseconds)
{
    m_player->setPosition(mseconds);
}

void Player::statusChanged(QMediaPlayer::MediaStatus status)
{
    handleCursor(status);

    // handle status message
    switch (status) {
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadedMedia:
        setStatusInfo(QString());
        break;
    case QMediaPlayer::LoadingMedia:
        setStatusInfo(tr("Loading..."));
        break;
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        setStatusInfo(tr("Buffering %1%").arg(qRound(m_player->bufferProgress() * 100.)));
        break;
    case QMediaPlayer::StalledMedia:
        setStatusInfo(tr("Stalled %1%").arg(qRound(m_player->bufferProgress() * 100.)));
        break;
    case QMediaPlayer::EndOfMedia:
        QApplication::alert(this);
        m_playlist->next();
        break;
    case QMediaPlayer::InvalidMedia:
        displayErrorMessage();
        break;
    }
}

void Player::handleCursor(QMediaPlayer::MediaStatus status)
{
#ifndef QT_NO_CURSOR
    if (status == QMediaPlayer::LoadingMedia || status == QMediaPlayer::BufferingMedia
        || status == QMediaPlayer::StalledMedia)
        setCursor(QCursor(Qt::BusyCursor));
    else
        unsetCursor();
#endif
}

void Player::bufferingProgress(float progress)
{
    if (m_player->mediaStatus() == QMediaPlayer::StalledMedia)
        setStatusInfo(tr("Stalled %1%").arg(qRound(progress * 100.)));
    else
        setStatusInfo(tr("Buffering %1%").arg(qRound(progress * 100.)));
}

void Player::videoAvailableChanged(bool available)
{
    if (!available) {
        disconnect(m_fullScreenButton, &QPushButton::clicked, m_videoWidget,
                   &QVideoWidget::setFullScreen);
        disconnect(m_videoWidget, &QVideoWidget::fullScreenChanged, m_fullScreenButton,
                   &QPushButton::setChecked);
        m_videoWidget->setFullScreen(false);
    } else {
        connect(m_fullScreenButton, &QPushButton::clicked, m_videoWidget,
                &QVideoWidget::setFullScreen);
        connect(m_videoWidget, &QVideoWidget::fullScreenChanged, m_fullScreenButton,
                &QPushButton::setChecked);

        if (m_fullScreenButton->isChecked())
            m_videoWidget->setFullScreen(true);
    }
}

void Player::selectAudioStream()
{
    int stream = m_audioTracks->currentData().toInt();
    m_player->setActiveAudioTrack(stream);
}

void Player::selectVideoStream()
{
    int stream = m_videoTracks->currentData().toInt();
    m_player->setActiveVideoTrack(stream);
}

void Player::selectSubtitleStream()
{
    int stream = m_subtitleTracks->currentData().toInt();
    m_player->setActiveSubtitleTrack(stream);
}

void Player::setTrackInfo(const QString &info)
{
    m_trackInfo = info;

    if (m_statusBar) {
        m_statusBar->showMessage(m_trackInfo);
        m_statusLabel->setText(m_statusInfo);
    } else {
        if (!m_statusInfo.isEmpty())
            setWindowTitle(QStringLiteral("%1 | %2").arg(m_trackInfo).arg(m_statusInfo));
        else
            setWindowTitle(m_trackInfo);
    }
}

void Player::setStatusInfo(const QString &info)
{
    m_statusInfo = info;

    if (m_statusBar) {
        m_statusBar->showMessage(m_trackInfo);
        m_statusLabel->setText(m_statusInfo);
    } else {
        if (!m_statusInfo.isEmpty())
            setWindowTitle(QStringLiteral("%1 | %2").arg(m_trackInfo).arg(m_statusInfo));
        else
            setWindowTitle(m_trackInfo);
    }
}

void Player::displayErrorMessage()
{
    if (m_player->error() == QMediaPlayer::NoError)
        return;
    setStatusInfo(m_player->errorString());
}

void Player::updateDurationInfo(qint64 currentInfo)
{
    QString tStr;
    if (currentInfo || m_duration) {
        QTime currentTime((currentInfo / 3600) % 60, (currentInfo / 60) % 60, currentInfo % 60,
                          (currentInfo * 1000) % 1000);
        QTime totalTime((m_duration / 3600) % 60, (m_duration / 60) % 60, m_duration % 60,
                        (m_duration * 1000) % 1000);
        QString format = "mm:ss";
        if (m_duration > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    m_labelDuration->setText(tStr);
}

void Player::updateAudioDevices()
{
    m_audioOutputCombo->clear();

    m_audioOutputCombo->addItem(QStringLiteral("Default"), QVariant::fromValue(QAudioDevice()));
    for (auto &deviceInfo : QMediaDevices::audioOutputs())
        m_audioOutputCombo->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
}

void Player::audioOutputChanged(int index)
{
    auto device = m_audioOutputCombo->itemData(index).value<QAudioDevice>();
    m_player->audioOutput()->setDevice(device);
}

#include "moc_player.cpp"

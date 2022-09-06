// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QAudioEngine>
#include <QAudioListener>
#include <QAudioRoom>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLineParser>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLibraryInfo>
#include <QLineEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSlider>
#include <QSpatialSound>
#include <QStandardPaths>

class AudioWidget : public QWidget
{
public:
    AudioWidget();
    void setFile(const QString &file);

private slots:
    void updatePosition();
    void newOcclusion();
    void modeChanged();
    void fileChanged(const QString &file);
    void openFileDialog();
    void updateRoom();
    void animateChanged();

private:
    QLineEdit *fileEdit = nullptr;
    QPushButton *fileDialogButton = nullptr;
    QSlider *azimuth = nullptr;
    QSlider *elevation = nullptr;
    QSlider *distance = nullptr;
    QSlider *occlusion = nullptr;
    QSlider *roomDimension = nullptr;
    QSlider *reverbGain = nullptr;
    QSlider *reflectionGain = nullptr;
    QComboBox *mode = nullptr;
    QCheckBox *animateButton = nullptr;
    QPropertyAnimation *animation = nullptr;

    QAudioEngine engine;
    QAudioListener *listener = nullptr;
    QSpatialSound *sound = nullptr;
    QAudioRoom *room = nullptr;
    QFileDialog *fileDialog = nullptr;
};

AudioWidget::AudioWidget()
    : QWidget()
{
    setMinimumSize(400, 300);
    auto *form = new QFormLayout(this);

    auto *fileLayout = new QHBoxLayout;
    fileEdit = new QLineEdit;
    fileEdit->setPlaceholderText(tr("Audio File"));
    fileLayout->addWidget(fileEdit);
    fileDialogButton = new QPushButton(tr("Choose..."));
    fileLayout->addWidget(fileDialogButton);
    form->addRow(fileLayout);

    azimuth = new QSlider(Qt::Horizontal);
    azimuth->setRange(-180, 180);
    form->addRow(tr("Azimuth (-180 - 180 degree):"), azimuth);

    elevation = new QSlider(Qt::Horizontal);
    elevation->setRange(-90, 90);
    form->addRow(tr("Elevation (-90 - 90 degree)"), elevation);

    distance = new QSlider(Qt::Horizontal);
    distance->setRange(0, 1000);
    distance->setValue(100);
    form->addRow(tr("Distance (0 - 10 meter):"), distance);

    occlusion = new QSlider(Qt::Horizontal);
    occlusion->setRange(0, 400);
    form->addRow(tr("Occlusion (0 - 4):"), occlusion);

    roomDimension = new QSlider(Qt::Horizontal);
    roomDimension->setRange(0, 10000);
    roomDimension->setValue(1000);
    form->addRow(tr("Room dimension (0 - 100 meter):"), roomDimension);

    reverbGain = new QSlider(Qt::Horizontal);
    reverbGain->setRange(0, 500);
    reverbGain->setValue(0);
    form->addRow(tr("Reverb gain (0-5):"), reverbGain);

    reflectionGain = new QSlider(Qt::Horizontal);
    reflectionGain->setRange(0, 500);
    reflectionGain->setValue(0);
    form->addRow(tr("Reflection gain (0-5):"), reflectionGain);

    mode = new QComboBox;
    mode->addItem(tr("Surround"), QVariant::fromValue(QAudioEngine::Surround));
    mode->addItem(tr("Stereo"), QVariant::fromValue(QAudioEngine::Stereo));
    mode->addItem(tr("Headphone"), QVariant::fromValue(QAudioEngine::Headphone));

    form->addRow(tr("Output mode:"), mode);

    animateButton = new QCheckBox(tr("Animate sound position"));
    form->addRow(animateButton);

    connect(fileEdit, &QLineEdit::textChanged, this, &AudioWidget::fileChanged);
    connect(fileDialogButton, &QPushButton::clicked, this, &AudioWidget::openFileDialog);

    connect(azimuth, &QSlider::valueChanged, this, &AudioWidget::updatePosition);
    connect(elevation, &QSlider::valueChanged, this, &AudioWidget::updatePosition);
    connect(distance, &QSlider::valueChanged, this, &AudioWidget::updatePosition);
    connect(occlusion, &QSlider::valueChanged, this, &AudioWidget::newOcclusion);

    connect(roomDimension, &QSlider::valueChanged, this, &AudioWidget::updateRoom);
    connect(reverbGain, &QSlider::valueChanged, this, &AudioWidget::updateRoom);
    connect(reflectionGain, &QSlider::valueChanged, this, &AudioWidget::updateRoom);

    connect(mode, &QComboBox::currentIndexChanged, this, &AudioWidget::modeChanged);

    room = new QAudioRoom(&engine);
    room->setWallMaterial(QAudioRoom::BackWall, QAudioRoom::BrickBare);
    room->setWallMaterial(QAudioRoom::FrontWall, QAudioRoom::BrickBare);
    room->setWallMaterial(QAudioRoom::LeftWall, QAudioRoom::BrickBare);
    room->setWallMaterial(QAudioRoom::RightWall, QAudioRoom::BrickBare);
    room->setWallMaterial(QAudioRoom::Floor, QAudioRoom::Marble);
    room->setWallMaterial(QAudioRoom::Ceiling, QAudioRoom::WoodCeiling);
    updateRoom();

    listener = new QAudioListener(&engine);
    listener->setPosition({});
    listener->setRotation({});
    engine.start();

    sound = new QSpatialSound(&engine);
    updatePosition();

    animation = new QPropertyAnimation(azimuth, "value");
    animation->setDuration(10000);
    animation->setStartValue(-180);
    animation->setEndValue(180);
    animation->setLoopCount(-1);
    connect(animateButton, &QCheckBox::toggled, this, &AudioWidget::animateChanged);
}

void AudioWidget::setFile(const QString &file)
{
    fileEdit->setText(file);
}

void AudioWidget::updatePosition()
{
    const float az = azimuth->value() / 180. * M_PI;
    const float el = elevation->value() / 180. * M_PI;
    const float d = distance->value();

    const float x = d * sin(az) * cos(el);
    const float y = d * sin(el);
    const float z = -d * cos(az) * cos(el);
    sound->setPosition({x, y, z});
}

void AudioWidget::newOcclusion()
{
    sound->setOcclusionIntensity(occlusion->value() / 100.);
}

void AudioWidget::modeChanged()
{
    engine.setOutputMode(mode->currentData().value<QAudioEngine::OutputMode>());
}

void AudioWidget::fileChanged(const QString &file)
{
    sound->setSource(QUrl::fromLocalFile(file));
    sound->setSize(5);
    sound->setLoops(QSpatialSound::Infinite);
}

void AudioWidget::openFileDialog()
{
    if (fileDialog == nullptr) {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        fileDialog = new QFileDialog(this, tr("Open Audio File"), dir);
        fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
        const QStringList mimeTypes{"audio/mpeg", "audio/aac", "audio/x-ms-wma",
                                    "audio/x-flac+ogg", "audio/x-wav"};
        fileDialog->setMimeTypeFilters(mimeTypes);
        fileDialog->selectMimeTypeFilter(mimeTypes.constFirst());
    }

    if (fileDialog->exec() == QDialog::Accepted)
        fileEdit->setText(fileDialog->selectedFiles().constFirst());
}

void AudioWidget::updateRoom()
{
    const float d = roomDimension->value();
    room->setDimensions(QVector3D(d, d, 400));
    room->setReflectionGain(float(reflectionGain->value()) / 100);
    room->setReverbGain(float(reverbGain->value()) / 100);
}

void AudioWidget::animateChanged()
{
    if (animateButton->isChecked())
        animation->start();
    else
        animation->stop();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationVersion(qVersion());
    QGuiApplication::setApplicationDisplayName(AudioWidget::tr("Spatial Audio test application"));

    QCommandLineParser commandLineParser;
    commandLineParser.addVersionOption();
    commandLineParser.addHelpOption();
    commandLineParser.addPositionalArgument("Audio File",
                                            "Audio File to play");

    commandLineParser.process(app);

    AudioWidget w;
    w.show();

    if (!commandLineParser.positionalArguments().isEmpty())
        w.setFile(commandLineParser.positionalArguments().constFirst());

    return app.exec();
}

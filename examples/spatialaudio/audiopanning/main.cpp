// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtWidgets/QtWidgets>
#include <QtSpatialAudio/QtSpatialAudio>
#include <QtCore/QPropertyAnimation>

class AudioWidget : public QWidget
{
public:
    AudioWidget()
        : QWidget()
    {
        setWindowTitle(tr("Spatial Audio test application"));

        setMinimumSize(400, 300);
        auto *grid = new QGridLayout(this);
        fileEdit = new QLineEdit;
        fileDialogButton = new QPushButton(tr("Open Filedialog"));
        grid->addWidget(fileEdit, 0, 0);
        grid->addWidget(fileDialogButton, 0, 1);

        azimuth = new QSlider(Qt::Horizontal);
        azimuth->setRange(-180, 180);
        grid->addWidget(new QLabel(tr("Azimuth (-180 - 180 degree):")), 1, 0);
        grid->addWidget(azimuth, 1, 1);
        elevation = new QSlider(Qt::Horizontal);
        elevation->setRange(-90, 90);
        grid->addWidget(new QLabel(tr("Elevation (-90 - 90 degree)")), 2, 0);
        grid->addWidget(elevation, 2, 1);
        distance = new QSlider(Qt::Horizontal);
        distance->setRange(0, 1000);
        distance->setValue(100);
        grid->addWidget(new QLabel(tr("Distance (0 - 10 meter):")), 3, 0);
        grid->addWidget(distance, 3, 1);
        occlusion = new QSlider(Qt::Horizontal);
        occlusion->setRange(0, 400);
        grid->addWidget(new QLabel(tr("Occlusion (0 - 4):")), 4, 0);
        grid->addWidget(occlusion, 4, 1);

        roomDimension = new QSlider(Qt::Horizontal);
        roomDimension->setRange(0, 10000);
        roomDimension->setValue(1000);
        grid->addWidget(new QLabel(tr("Room dimension (0 - 100 meter):")), 5, 0);
        grid->addWidget(roomDimension, 5, 1);

        reverbGain = new QSlider(Qt::Horizontal);
        reverbGain->setRange(0, 500);
        reverbGain->setValue(0);
        grid->addWidget(new QLabel(tr("Reverb gain (0-5):")), 6, 0);
        grid->addWidget(reverbGain, 6, 1);

        reflectionGain = new QSlider(Qt::Horizontal);
        reflectionGain->setRange(0, 500);
        reflectionGain->setValue(0);
        grid->addWidget(new QLabel(tr("Reflection gain (0-5):")), 7, 0);
        grid->addWidget(reflectionGain, 7, 1);

        mode = new QComboBox;
        mode->addItem(tr("Surround"), QVariant::fromValue(QAudioEngine::Surround));
        mode->addItem(tr("Stereo"), QVariant::fromValue(QAudioEngine::Stereo));
        mode->addItem(tr("Headphone"), QVariant::fromValue(QAudioEngine::Headphone));
        grid->addWidget(new QLabel(tr("Output mode:")), 8, 0);
        grid->addWidget(mode, 8, 1);

        animateButton = new QCheckBox(tr("Animate sound position"));
        grid->addWidget(animateButton, 9, 0);

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
    void setFile(const QString &file) { fileEdit->setText(file); }
private slots:
    void updatePosition()
    {
        float az = azimuth->value()/180.*M_PI;
        float el = elevation->value()/180.*M_PI;
        float d = distance->value();

        float x = d*sin(az)*cos(el);
        float y = d*sin(el);
        float z = -d*cos(az)*cos(el);
        sound->setPosition({x, y, z});
    }
    void newOcclusion()
    {
        sound->setOcclusionIntensity(occlusion->value()/100.);
    }
    void modeChanged()
    {
        engine.setOutputMode(mode->currentData().value<QAudioEngine::OutputMode>());
    }
    void fileChanged(const QString &file)
    {
        sound->setSource(QUrl::fromLocalFile(file));
        sound->setSize(5);
        sound->setLoops(QSpatialSound::Infinite);
    }
    void openFileDialog()
    {
        auto file = QFileDialog::getOpenFileName(this);
        fileEdit->setText(file);
    }
    void updateRoom()
    {
        float d = roomDimension->value();
        room->setDimensions(QVector3D(d, d, 400));
        room->setReflectionGain(float(reflectionGain->value())/100);
        room->setReverbGain(float(reverbGain->value())/100);
    }
    void animateChanged()
    {
        if (animateButton->isChecked())
            animation->start();
        else
            animation->stop();
    }

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
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    AudioWidget *w = new AudioWidget;
    w->show();
    if (argc > 1) {
        auto file = QString::fromUtf8(argv[1]);
        w->setFile(file);
    }

    return app.exec();
}

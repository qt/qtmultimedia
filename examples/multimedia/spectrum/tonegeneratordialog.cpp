// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "tonegeneratordialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

const int ToneGeneratorFreqMin = 1;
const int ToneGeneratorFreqMax = 1000;
const int ToneGeneratorFreqDefault = 440;
const int ToneGeneratorAmplitudeDefault = 75;

ToneGeneratorDialog::ToneGeneratorDialog(QWidget *parent)
    : QDialog(parent),
      m_toneGeneratorSweepCheckBox(new QCheckBox(tr("Frequency sweep"), this)),
      m_frequencySweepEnabled(true),
      m_toneGeneratorControl(new QWidget(this)),
      m_toneGeneratorFrequencyControl(new QWidget(this)),
      m_frequencySlider(new QSlider(Qt::Horizontal, this)),
      m_frequencySpinBox(new QSpinBox(this)),
      m_amplitudeSlider(new QSlider(Qt::Horizontal, this))
{
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);

    m_toneGeneratorSweepCheckBox->setChecked(true);

    // Configure tone generator controls
    m_frequencySlider->setRange(ToneGeneratorFreqMin, ToneGeneratorFreqMax);
    m_frequencySlider->setValue(ToneGeneratorFreqDefault);
    m_frequencySpinBox->setRange(ToneGeneratorFreqMin, ToneGeneratorFreqMax);
    m_frequencySpinBox->setValue(ToneGeneratorFreqDefault);
    m_amplitudeSlider->setRange(0, 100);
    m_amplitudeSlider->setValue(ToneGeneratorAmplitudeDefault);

    // Add widgets to layout
    QGridLayout *frequencyControlLayout = new QGridLayout;
    QLabel *frequencyLabel = new QLabel(tr("Frequency (Hz)"), this);
    frequencyControlLayout->addWidget(frequencyLabel, 0, 0, 2, 1);
    frequencyControlLayout->addWidget(m_frequencySlider, 0, 1);
    frequencyControlLayout->addWidget(m_frequencySpinBox, 1, 1);
    m_toneGeneratorFrequencyControl->setLayout(frequencyControlLayout);
    m_toneGeneratorFrequencyControl->setEnabled(false);

    QGridLayout *toneGeneratorLayout = new QGridLayout;
    QLabel *amplitudeLabel = new QLabel(tr("Amplitude"), this);
    toneGeneratorLayout->addWidget(m_toneGeneratorSweepCheckBox, 0, 1);
    toneGeneratorLayout->addWidget(m_toneGeneratorFrequencyControl, 1, 0, 1, 2);
    toneGeneratorLayout->addWidget(amplitudeLabel, 2, 0);
    toneGeneratorLayout->addWidget(m_amplitudeSlider, 2, 1);
    m_toneGeneratorControl->setLayout(toneGeneratorLayout);
    m_toneGeneratorControl->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    dialogLayout->addWidget(m_toneGeneratorControl);

    // Connect
    connect(m_toneGeneratorSweepCheckBox, &QCheckBox::toggled, this,
            &ToneGeneratorDialog::frequencySweepEnabled);
    connect(m_frequencySlider, &QSlider::valueChanged, m_frequencySpinBox, &QSpinBox::setValue);
    connect(m_frequencySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_frequencySlider,
            &QSlider::setValue);

    // Add standard buttons to layout
    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    dialogLayout->addWidget(buttonBox);

    // Connect standard buttons
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this,
            &ToneGeneratorDialog::accept);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this,
            &ToneGeneratorDialog::reject);

    setLayout(dialogLayout);
}

ToneGeneratorDialog::~ToneGeneratorDialog() = default;

bool ToneGeneratorDialog::isFrequencySweepEnabled() const
{
    return m_toneGeneratorSweepCheckBox->isChecked();
}

qreal ToneGeneratorDialog::frequency() const
{
    return qreal(m_frequencySlider->value());
}

qreal ToneGeneratorDialog::amplitude() const
{
    return qreal(m_amplitudeSlider->value()) / 100.0;
}

void ToneGeneratorDialog::frequencySweepEnabled(bool enabled)
{
    m_frequencySweepEnabled = enabled;
    m_toneGeneratorFrequencyControl->setEnabled(!enabled);
}

#include "moc_tonegeneratordialog.cpp"

// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "settingsdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(const QList<QAudioDevice> &availableInputDevices,
                               const QList<QAudioDevice> &availableOutputDevices, QWidget *parent)
    : QDialog(parent),
      m_windowFunction(DefaultWindowFunction),
      m_inputDeviceComboBox(new QComboBox(this)),
      m_outputDeviceComboBox(new QComboBox(this)),
      m_windowFunctionComboBox(new QComboBox(this))
{
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);

    // Populate combo boxes

    for (const QAudioDevice &device : availableInputDevices)
        m_inputDeviceComboBox->addItem(device.description(), QVariant::fromValue(device));
    for (const QAudioDevice &device : availableOutputDevices)
        m_outputDeviceComboBox->addItem(device.description(), QVariant::fromValue(device));

    m_windowFunctionComboBox->addItem(tr("None"), QVariant::fromValue(int(NoWindow)));
    m_windowFunctionComboBox->addItem("Hann", QVariant::fromValue(int(HannWindow)));
    m_windowFunctionComboBox->setCurrentIndex(m_windowFunction);

    // Initialize default devices
    if (!availableInputDevices.empty())
        m_inputDevice = availableInputDevices.front();
    if (!availableOutputDevices.empty())
        m_outputDevice = availableOutputDevices.front();

    // Add widgets to layout

    std::unique_ptr<QHBoxLayout> inputDeviceLayout(new QHBoxLayout);
    QLabel *inputDeviceLabel = new QLabel(tr("Input device"), this);
    inputDeviceLayout->addWidget(inputDeviceLabel);
    inputDeviceLayout->addWidget(m_inputDeviceComboBox);
    dialogLayout->addLayout(inputDeviceLayout.release());

    std::unique_ptr<QHBoxLayout> outputDeviceLayout(new QHBoxLayout);
    QLabel *outputDeviceLabel = new QLabel(tr("Output device"), this);
    outputDeviceLayout->addWidget(outputDeviceLabel);
    outputDeviceLayout->addWidget(m_outputDeviceComboBox);
    dialogLayout->addLayout(outputDeviceLayout.release());

    std::unique_ptr<QHBoxLayout> windowFunctionLayout(new QHBoxLayout);
    QLabel *windowFunctionLabel = new QLabel(tr("Window function"), this);
    windowFunctionLayout->addWidget(windowFunctionLabel);
    windowFunctionLayout->addWidget(m_windowFunctionComboBox);
    dialogLayout->addLayout(windowFunctionLayout.release());

    // Connect
    connect(m_inputDeviceComboBox, QOverload<int>::of(&QComboBox::activated), this,
            &SettingsDialog::inputDeviceChanged);
    connect(m_outputDeviceComboBox, QOverload<int>::of(&QComboBox::activated), this,
            &SettingsDialog::outputDeviceChanged);
    connect(m_windowFunctionComboBox, QOverload<int>::of(&QComboBox::activated), this,
            &SettingsDialog::windowFunctionChanged);

    // Add standard buttons to layout
    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    dialogLayout->addWidget(buttonBox);

    // Connect standard buttons
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this,
            &SettingsDialog::accept);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this,
            &SettingsDialog::reject);

    setLayout(dialogLayout);
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::windowFunctionChanged(int index)
{
    m_windowFunction =
            static_cast<WindowFunction>(m_windowFunctionComboBox->itemData(index).value<int>());
}

void SettingsDialog::inputDeviceChanged(int index)
{
    m_inputDevice = m_inputDeviceComboBox->itemData(index).value<QAudioDevice>();
}

void SettingsDialog::outputDeviceChanged(int index)
{
    m_outputDevice = m_outputDeviceComboBox->itemData(index).value<QAudioDevice>();
}

#include "moc_settingsdialog.cpp"

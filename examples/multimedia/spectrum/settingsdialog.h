// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "spectrum.h"

#include <QAudioDevice>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
class QSlider;
class QSpinBox;
class QGridLayout;
QT_END_NAMESPACE

/**
 * Dialog used to control settings such as the audio input / output device
 * and the windowing function.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(const QList<QAudioDevice> &availableInputDevices,
                   const QList<QAudioDevice> &availableOutputDevices, QWidget *parent = nullptr);
    ~SettingsDialog();

    WindowFunction windowFunction() const { return m_windowFunction; }
    const QAudioDevice &inputDevice() const { return m_inputDevice; }
    const QAudioDevice &outputDevice() const { return m_outputDevice; }

private slots:
    void windowFunctionChanged(int index);
    void inputDeviceChanged(int index);
    void outputDeviceChanged(int index);

private:
    WindowFunction m_windowFunction;
    QAudioDevice m_inputDevice;
    QAudioDevice m_outputDevice;

    QComboBox *m_inputDeviceComboBox;
    QComboBox *m_outputDeviceComboBox;
    QComboBox *m_windowFunctionComboBox;
};

#endif // SETTINGSDIALOG_H

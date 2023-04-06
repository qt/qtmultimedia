// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TONEGENERATORDIALOG_H
#define TONEGENERATORDIALOG_H

#include "spectrum.h"

#include <QAudioDevice>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QSlider;
class QSpinBox;
class QGridLayout;
QT_END_NAMESPACE

/**
 * Dialog which controls the parameters of the tone generator.
 */
class ToneGeneratorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ToneGeneratorDialog(QWidget *parent = nullptr);
    ~ToneGeneratorDialog();

    bool isFrequencySweepEnabled() const;
    qreal frequency() const;
    qreal amplitude() const;

private slots:
    void frequencySweepEnabled(bool enabled);

private:
    QCheckBox *m_toneGeneratorSweepCheckBox;
    bool m_frequencySweepEnabled;
    QWidget *m_toneGeneratorControl;
    QWidget *m_toneGeneratorFrequencyControl;
    QSlider *m_frequencySlider;
    QSpinBox *m_frequencySpinBox;
    QSlider *m_amplitudeSlider;
};

#endif // TONEGENERATORDIALOG_H

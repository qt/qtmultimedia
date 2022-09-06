// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TONEGENERATOR_H
#define TONEGENERATOR_H

#include "spectrum.h"

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QAudioFormat;
class QByteArray;
QT_END_NAMESPACE

/**
 * Generate a sine wave
 */
void generateTone(const SweptTone &tone, const QAudioFormat &format, QByteArray &buffer);

#endif // TONEGENERATOR_H

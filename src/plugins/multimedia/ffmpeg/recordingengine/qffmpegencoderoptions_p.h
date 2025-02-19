// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGENCODEROPTIONS_P_H
#define QFFMPEGENCODEROPTIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#include "qvideoframeformat.h"
#include <QtMultimedia/private/qplatformmediarecorder_p.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

void applyVideoEncoderOptions(const QMediaEncoderSettings &settings, const QByteArray &codecName, AVCodecContext *codec, AVDictionary **opts);
void applyAudioEncoderOptions(const QMediaEncoderSettings &settings, const QByteArray &codecName, AVCodecContext *codec, AVDictionary **opts);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif

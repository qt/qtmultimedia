/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QPULSEHELPER_H
#define QPULSEHELPER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qaudiodeviceinfo.h"
#include <qaudioformat.h>
#include <pulse/pulseaudio.h>

QT_BEGIN_NAMESPACE

namespace QPulseAudioInternal
{
pa_sample_spec audioFormatToSampleSpec(const QAudioFormat &format);
QString stateToQString(pa_stream_state_t state);
QString stateToQString(pa_context_state_t state);
QString sampleFormatToQString(pa_sample_format format);
QAudioFormat sampleSpecToAudioFormat(pa_sample_spec spec);
}

QT_END_NAMESPACE

#endif

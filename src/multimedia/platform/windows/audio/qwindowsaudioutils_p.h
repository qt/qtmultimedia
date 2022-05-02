/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QWINDOWSAUDIOUTILS_H
#define QWINDOWSAUDIOUTILS_H

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

#include <qaudioformat.h>
#include <QtCore/qt_windows.h>
#include <private/qwindowsiupointer_p.h>
#include <mfapi.h>
#include <mmsystem.h>
#include <mmreg.h>

struct IMFMediaType;

QT_BEGIN_NAMESPACE

namespace QWindowsAudioUtils
{
    bool formatToWaveFormatExtensible(const QAudioFormat &format, WAVEFORMATEXTENSIBLE &wfx);
    QAudioFormat waveFormatExToFormat(const WAVEFORMATEX &in);
    QAudioFormat mediaTypeToFormat(IMFMediaType *mediaType);
    QWindowsIUPointer<IMFMediaType> formatToMediaType(const QAudioFormat &format);

}

QT_END_NAMESPACE

#endif // QWINDOWSAUDIOUTILS_H

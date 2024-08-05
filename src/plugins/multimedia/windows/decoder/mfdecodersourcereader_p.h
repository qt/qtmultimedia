// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MFDECODERSOURCEREADER_H
#define MFDECODERSOURCEREADER_H

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

#include <mfidl.h>
#include <mfreadwrite.h>

#include <QtCore/qobject.h>
#include "qaudioformat.h"
#include <QtCore/private/qcomptr_p.h>
#include "mfdecodersourcereadercallback_p.h"

QT_BEGIN_NAMESPACE

class MFDecoderSourceReader : public QObject
{
    Q_OBJECT
public:
    void clearSource() { m_sourceReader.Reset(); }
    ComPtr<IMFMediaType> setSource(IMFMediaSource *source, QAudioFormat::SampleFormat);

    void readNextSample();

Q_SIGNALS:
    void newSample(ComPtr<IMFSample>);
    void finished();

private:
    ComPtr<IMFSourceReader> m_sourceReader;
    ComPtr<MFSourceReaderCallback> m_sourceReaderCallback;
};

QT_END_NAMESPACE

#endif//MFDECODERSOURCEREADER_H

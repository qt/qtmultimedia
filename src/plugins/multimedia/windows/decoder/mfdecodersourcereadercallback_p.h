// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MFDECODERSOURCEREADERCALLBACK_H
#define MFDECODERSOURCEREADERCALLBACK_H

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
#include <QtCore/private/qcomptr_p.h>
#include <QtCore/private/qcomobject_p.h>

QT_BEGIN_NAMESPACE

class MFSourceReaderCallback : public QObject, public QComObject<IMFSourceReaderCallback>
{
    Q_OBJECT
public:
    STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags,
                              LONGLONG llTimestamp, IMFSample *pSample) override;
    STDMETHODIMP OnFlush(DWORD) override { return S_OK; }
    STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *) override { return S_OK; }

Q_SIGNALS:
    void newSample(ComPtr<IMFSample>);
    void finished();
};

QT_END_NAMESPACE

#endif // MFDECODERSOURCEREADERCALLBACK_H

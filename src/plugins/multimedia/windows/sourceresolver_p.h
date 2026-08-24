// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef SOURCERESOLVER_H
#define SOURCERESOLVER_H

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

#include <mfstream_p.h>

#include <QtCore/qurl.h>
#include <QtCore/private/qcomobject_p.h>
#include <QtCore/private/qcomptr_p.h>

QT_BEGIN_NAMESPACE

class SourceResolver : public QObject,
                       public QComObjectWithDeleteLater<SourceResolver, IMFAsyncCallback>
{
    Q_OBJECT
public:
    SourceResolver();
    ~SourceResolver();

    HRESULT STDMETHODCALLTYPE Invoke(IMFAsyncResult *pAsyncResult) override;

    HRESULT STDMETHODCALLTYPE GetParameters(DWORD*, DWORD*) override;

    void load(const QUrl &url, QIODevice* stream);

    void cancel();

    void shutdown();

    ComPtr<IMFMediaSource> mediaSource() const;

Q_SIGNALS:
    void error(long hr);
    void mediaSourceReady();

private:
    class State : public QComObject<IUnknown>
    {
    public:
        State(ComPtr<IMFSourceResolver> sourceResolver, bool fromStream);
        virtual ~State();

        ComPtr<IMFSourceResolver> sourceResolver() const;
        bool fromStream() const;

    private:
        ComPtr<IMFSourceResolver> m_sourceResolver;
        bool m_fromStream;
    };

    ComPtr<IUnknown> m_cancelCookie;
    ComPtr<IMFSourceResolver> m_sourceResolver;
    ComPtr<IMFMediaSource> m_mediaSource;
    ComPtr<MFStream> m_stream;
    mutable QMutex m_mutex;
};

QT_END_NAMESPACE

#endif

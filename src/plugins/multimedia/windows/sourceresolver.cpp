// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "sourceresolver_p.h"

#include <mfstream_p.h>

#include <QtMultimedia/qmediaplayer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>

#include <mferror.h>
#include <nserror.h>

QT_BEGIN_NAMESPACE

/*
    SourceResolver is separated from MFPlayerSession to handle the work of resolving a media source
    asynchronously. You call SourceResolver::load to request resolving a media source asynchronously,
    and it will emit mediaSourceReady() when resolving is done. You can call SourceResolver::cancel to
    stop the previous load operation if there is any.
*/

SourceResolver::SourceResolver() = default;

SourceResolver::~SourceResolver()
{
    shutdown();
}


HRESULT STDMETHODCALLTYPE SourceResolver::Invoke(IMFAsyncResult *pAsyncResult)
{
    QMutexLocker locker(&m_mutex);

    if (!m_sourceResolver)
        return S_OK;

    MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
    ComPtr<IUnknown> pSource;
    State *state = static_cast<State*>(pAsyncResult->GetStateNoAddRef());

    HRESULT hr = S_OK;
    if (state->fromStream())
        hr = m_sourceResolver->EndCreateObjectFromByteStream(pAsyncResult, &ObjectType, &pSource);
    else
        hr = m_sourceResolver->EndCreateObjectFromURL(pAsyncResult, &ObjectType, &pSource);

    if (state->sourceResolver() != m_sourceResolver) {
        //This is a cancelled one
        return S_OK;
    }

    m_cancelCookie = nullptr;

    if (FAILED(hr)) {
        emit error(hr);
        return S_OK;
    }

    hr = pSource.As(&m_mediaSource);
    if (FAILED(hr)) {
        emit error(hr);
        return S_OK;
    }

    emit mediaSourceReady();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE SourceResolver::GetParameters(DWORD*, DWORD*)
{
    return E_NOTIMPL;
}

void SourceResolver::load(const QUrl &url, QIODevice* stream)
{
    QMutexLocker locker(&m_mutex);
    HRESULT hr = S_OK;
    if (!m_sourceResolver)
        hr = MFCreateSourceResolver(&m_sourceResolver);

    m_stream = nullptr;

    if (FAILED(hr)) {
        qWarning() << "Failed to create Source Resolver!";
        emit error(hr);
    } else if (stream) {
        QString urlString = url.toString();
        m_stream = makeComObject<MFStream>(stream, false);
        hr = m_sourceResolver->BeginCreateObjectFromByteStream(
                    m_stream.Get(), urlString.isEmpty() ? 0 : reinterpret_cast<LPCWSTR>(urlString.utf16()),
                    MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE
                    , NULL, &m_cancelCookie, this, makeComObject<State>(m_sourceResolver, true).Get());
        if (FAILED(hr)) {
            qWarning() << "Unsupported stream!";
            emit error(hr);
        }
    } else {
#ifdef DEBUG_MEDIAFOUNDATION
        qDebug() << "loading :" << url;
        qDebug() << "url path =" << url.path().mid(1);
#endif
#ifdef TEST_STREAMING
        //Testing stream function
        if (url.scheme() == QLatin1String("file")) {
            stream = new QFile(url.path().mid(1));
            if (stream->open(QIODevice::ReadOnly)) {
                m_stream = makeComObject<MFStream>(stream, true);
                hr = m_sourceResolver->BeginCreateObjectFromByteStream(
                            m_stream.Get(), reinterpret_cast<const OLECHAR *>(url.toString().utf16()),
                            MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
                            NULL, &m_cancelCookie, this, makeComObject<State>(m_sourceResolver, true).Get());
                if (FAILED(hr)) {
                    qWarning() << "Unsupported stream!";
                    emit error(hr);
                }
            } else {
                delete stream;
                emit error(QMediaPlayer::FormatError);
            }
        } else
#endif
        if (url.scheme() == QLatin1String("qrc")) {
            // If the canonical URL refers to a Qt resource, open with QFile and use
            // the stream playback capability to play.
            stream = new QFile(QLatin1Char(':') + url.path());
            if (stream->open(QIODevice::ReadOnly)) {
                m_stream = makeComObject<MFStream>(stream, true);
                hr = m_sourceResolver->BeginCreateObjectFromByteStream(
                            m_stream.Get(), reinterpret_cast<const OLECHAR *>(url.toString().utf16()),
                            MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
                            NULL, &m_cancelCookie, this, makeComObject<State>(m_sourceResolver, true).Get());
                if (FAILED(hr)) {
                    qWarning() << "Unsupported stream!";
                    emit error(hr);
                }
            } else {
                delete stream;
                emit error(QMediaPlayer::FormatError);
            }
        } else {
            hr = m_sourceResolver->BeginCreateObjectFromURL(
                        reinterpret_cast<const OLECHAR *>(url.toString().utf16()),
                        MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
                        NULL, &m_cancelCookie, this, makeComObject<State>(m_sourceResolver, false).Get());
            if (FAILED(hr)) {
                qWarning() << "Unsupported url scheme!";
                emit error(hr);
            }
        }
    }
}

void SourceResolver::cancel()
{
    QMutexLocker locker(&m_mutex);
    if (m_cancelCookie) {
        m_sourceResolver->CancelObjectCreation(m_cancelCookie.Get());
        m_cancelCookie = nullptr;
        m_sourceResolver = nullptr;
    }
}

void SourceResolver::shutdown()
{
    if (m_mediaSource) {
        m_mediaSource->Shutdown();
        m_mediaSource = nullptr;
    }

    m_stream = nullptr;
}

ComPtr<IMFMediaSource> SourceResolver::mediaSource() const
{
    return m_mediaSource;
}

/////////////////////////////////////////////////////////////////////////////////
SourceResolver::State::State(ComPtr<IMFSourceResolver> sourceResolver, bool fromStream)
    : m_sourceResolver(std::move(sourceResolver)), m_fromStream(fromStream)
{
}

SourceResolver::State::~State() = default;

ComPtr<IMFSourceResolver> SourceResolver::State::sourceResolver() const
{
    return m_sourceResolver;
}

bool SourceResolver::State::fromStream() const
{
    return m_fromStream;
}

QT_END_NAMESPACE

#include "moc_sourceresolver_p.cpp"

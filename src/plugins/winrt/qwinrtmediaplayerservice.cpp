/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "qwinrtmediaplayerservice.h"
#include "qwinrtmediaplayercontrol.h"

#include <QtCore/qfunctions_winrt.h>
#include <QtCore/QPointer>
#include <QtMultimedia/QVideoRendererControl>

#include <mfapi.h>
#include <mfmediaengine.h>
#include <wrl.h>

using namespace Microsoft::WRL;

QT_BEGIN_NAMESPACE

class QWinRTMediaPlayerServicePrivate
{
public:
    QPointer<QWinRTMediaPlayerControl> player;

    ComPtr<IMFMediaEngineClassFactory> factory;
};

QWinRTMediaPlayerService::QWinRTMediaPlayerService(QObject *parent)
    : QMediaService(parent), d_ptr(new QWinRTMediaPlayerServicePrivate)
{
    Q_D(QWinRTMediaPlayerService);

    d->player = nullptr;

    HRESULT hr = MFStartup(MF_VERSION);
    Q_ASSERT(SUCCEEDED(hr));

    MULTI_QI results = { &IID_IUnknown, nullptr, 0 };
    hr = CoCreateInstanceFromApp(CLSID_MFMediaEngineClassFactory, nullptr,
                                 CLSCTX_INPROC_SERVER, nullptr, 1, &results);
    Q_ASSERT(SUCCEEDED(hr));

    hr = results.pItf->QueryInterface(d->factory.GetAddressOf());
    Q_ASSERT(SUCCEEDED(hr));
}

QWinRTMediaPlayerService::~QWinRTMediaPlayerService()
{
    MFShutdown();
}

QMediaControl *QWinRTMediaPlayerService::requestControl(const char *name)
{
    Q_D(QWinRTMediaPlayerService);
    if (qstrcmp(name, QMediaPlayerControl_iid) == 0) {
        if (!d->player)
            d->player = new QWinRTMediaPlayerControl(d->factory.Get(), this);
        return d->player;
    }
    if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
        if (!d->player)
            return nullptr;
        return d->player->videoRendererControl();
    }

    return nullptr;
}

void QWinRTMediaPlayerService::releaseControl(QMediaControl *control)
{
    Q_D(QWinRTMediaPlayerService);
    if (control == d->player)
        d->player->deleteLater();
}

QT_END_NAMESPACE

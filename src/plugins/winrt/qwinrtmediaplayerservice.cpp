/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
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

    d->player = Q_NULLPTR;

    HRESULT hr = MFStartup(MF_VERSION);
    Q_ASSERT(SUCCEEDED(hr));

    MULTI_QI results = { &IID_IUnknown, NULL, 0 };
    hr = CoCreateInstanceFromApp(CLSID_MFMediaEngineClassFactory, NULL,
                                 CLSCTX_INPROC_SERVER, NULL, 1, &results);
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
            return Q_NULLPTR;
        return d->player->videoRendererControl();
    }

    return Q_NULLPTR;
}

void QWinRTMediaPlayerService::releaseControl(QMediaControl *control)
{
    Q_D(QWinRTMediaPlayerService);
    if (control == d->player)
        d->player->deleteLater();
}

QT_END_NAMESPACE

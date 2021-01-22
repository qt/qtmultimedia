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

#ifndef DIRECTSHOWMEDIATYPEENUM_H
#define DIRECTSHOWMEDIATYPEENUM_H

#include "directshowobject.h"
#include <qlist.h>

QT_BEGIN_NAMESPACE

class DirectShowPin;
class DirectShowMediaType;

class DirectShowMediaTypeEnum : public IEnumMediaTypes
{
    COM_REF_MIXIN
public:
    DirectShowMediaTypeEnum(DirectShowPin *pin);
    DirectShowMediaTypeEnum(const QList<DirectShowMediaType> &types);
    virtual ~DirectShowMediaTypeEnum();

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override;

    // IEnumMediaTypes
    STDMETHODIMP Next(ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched) override;
    STDMETHODIMP Skip(ULONG cMediaTypes) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Clone(IEnumMediaTypes **ppEnum) override;

private:
    Q_DISABLE_COPY(DirectShowMediaTypeEnum)

    DirectShowPin *m_pin = nullptr;
    QList<DirectShowMediaType> m_mediaTypes;
    int m_index = 0;
};

QT_END_NAMESPACE

#endif // DIRECTSHOWMEDIATYPEENUM_H

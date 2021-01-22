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

#ifndef DIRECTSHOWPINENUM_H
#define DIRECTSHOWPINENUM_H

#include <dshow.h>

#include <QtCore/qlist.h>
#include "directshowpin.h"

QT_BEGIN_NAMESPACE

class DirectShowBaseFilter;

class DirectShowPinEnum : public IEnumPins
{
    COM_REF_MIXIN
public:
    DirectShowPinEnum(DirectShowBaseFilter *filter);
    DirectShowPinEnum(const QList<IPin *> &pins);
    virtual ~DirectShowPinEnum();

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override;

    // IEnumPins
    STDMETHODIMP Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched) override;
    STDMETHODIMP Skip(ULONG cPins) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Clone(IEnumPins **ppEnum) override;

private:
    Q_DISABLE_COPY(DirectShowPinEnum)

    DirectShowBaseFilter *m_filter = nullptr;
    QList<IPin *> m_pins;
    int m_index = 0;
};

QT_END_NAMESPACE

#endif

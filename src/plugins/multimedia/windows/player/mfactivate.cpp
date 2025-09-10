// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mfactivate_p.h"

#include <mfapi.h>

MFAbstractActivate::MFAbstractActivate()
{
    MFCreateAttributes(&m_attributes, 0);
}

MFAbstractActivate::~MFAbstractActivate()
{
    if (m_attributes)
        m_attributes->Release();
}

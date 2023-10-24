// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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

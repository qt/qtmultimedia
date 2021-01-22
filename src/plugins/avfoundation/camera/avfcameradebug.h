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

#ifndef AVFDEBUG_H
#define AVFDEBUG_H

#include "qtmultimediaglobal.h"

#include <QtCore/qdebug.h>

QT_USE_NAMESPACE

//#define AVF_DEBUG_CAMERA

#ifdef AVF_DEBUG_CAMERA
#define qDebugCamera qDebug
#else
#define qDebugCamera QT_NO_QDEBUG_MACRO
#endif

#endif

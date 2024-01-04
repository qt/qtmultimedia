// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore/QtGlobal>

#if defined(QMLVIDEO_LIB)
#define QMLVIDEO_LIB_EXPORT Q_DECL_EXPORT
#else
#define QMLVIDEO_LIB_EXPORT Q_DECL_IMPORT
#endif

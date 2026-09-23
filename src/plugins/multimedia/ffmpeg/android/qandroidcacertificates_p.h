// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDCACERTIFICATES_P_H
#define QANDROIDCACERTIFICATES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

// Returns the path to a PEM CA bundle derived from Android's system trust anchors, or an
// empty string on failure. The bundle is fetched from the system trust store via JNI once
// per process. The cache file is rewritten on the first call in each process, so a file left
// behind by a previous run cannot outlive a system trust store update, and again whenever it
// gets evicted from the cache directory while the process is still running.
QString androidFFmpegCaBundlePath();

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QANDROIDCACERTIFICATES_P_H

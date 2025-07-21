// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFSTILLPHOTONOTIFIER_P_H
#define QAVFSTILLPHOTONOTIFIER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qtconfigmacros.h>

#include <QtMultimedia/qimagecapture.h>
#include <QtMultimedia/qvideoframe.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

// Helper class for QAVFCapturePhotoOutputDelegate, that allows
// us to emit success/failure events from background threads
// in thread-safe manner.
class QAVFStillPhotoNotifier : public QObject {
    Q_OBJECT
signals:
    void succeeded(QVideoFrame);
    void failed(QImageCapture::Error, QString);
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QAVFSTILLPHOTONOTIFIER_P_H

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QLIBCAMERA_INSTANCE_P_H
#define QLIBCAMERA_INSTANCE_P_H

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

#include <memory>

namespace libcamera {
class Camera;
class CameraManager;
} // namespace libcamera

QT_BEGIN_NAMESPACE

namespace QLibCamera {

class QLibCameraInstance : public QObject
{
    Q_OBJECT
public:
    ~QLibCameraInstance() override;

    static std::shared_ptr<QLibCameraInstance> instance();

    libcamera::CameraManager *manager() const { return m_manager.get(); }

    std::shared_ptr<libcamera::Camera> get(const QString &id) const;

Q_SIGNALS:
    void cameraAdded(const QString &id);
    void cameraRemoved(const QString &id);

private:
    QLibCameraInstance();

    const std::unique_ptr<libcamera::CameraManager> m_manager;
    // Whether m_manager->start() succeeded -- set from its return value in the constructor, as
    // m_manager itself is always non-null. instance() and the destructor rely on this rather than
    // a null check on m_manager.
    bool m_started = false;
};

} // namespace QLibCamera

QT_END_NAMESPACE

#endif // QLIBCAMERA_INSTANCE_P_H

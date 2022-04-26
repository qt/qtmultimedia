/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QQnxCamera_H
#define QQnxCamera_H

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

#include <private/qplatformcamera_p.h>

#include <QtCore/qlist.h>

#include <camera/camera_api.h>

QT_BEGIN_NAMESPACE

class QQnxMediaCaptureSession;

class CameraHandle
{
public:
    CameraHandle() = default;

    explicit CameraHandle(camera_handle_t h)
        : m_handle (h) {}

    explicit CameraHandle(CameraHandle &&other)
        : m_handle(other.m_handle)
        , m_lastError(other.m_lastError)
    {
        other = CameraHandle();
    }

    CameraHandle(const CameraHandle&) = delete;

    CameraHandle& operator=(CameraHandle&& other)
    {
        m_handle = other.m_handle;
        m_lastError = other.m_lastError;

        other = CameraHandle();

        return *this;
    }

    ~CameraHandle()
    {
        close();
    }

    bool open(camera_unit_t unit, uint32_t mode)
    {
        if (isOpen()) {
            m_lastError = CAMERA_EALREADY;
            return false;
        }

        return cacheError(camera_open, unit, mode, &m_handle);
    }

    bool close()
    {
        if (!isOpen())
            return CAMERA_EOK;

        const bool success = cacheError(camera_close, m_handle);
        m_handle = CAMERA_HANDLE_INVALID;

        return success;
    }

    camera_handle_t get() const
    {
        return m_handle;
    }

    bool isOpen() const
    {
        return m_handle != CAMERA_HANDLE_INVALID;
    }

    camera_error_t lastError() const
    {
        return m_lastError;
    }

private:
    template <typename Func, typename ...Args>
    bool cacheError(Func f, Args &&...args)
    {
        m_lastError = f(std::forward<Args>(args)...);

        return m_lastError == CAMERA_EOK;
    }

    camera_handle_t m_handle = CAMERA_HANDLE_INVALID;
    camera_error_t m_lastError = CAMERA_EOK;
};


class QQnxCamera : public QPlatformCamera
{
    Q_OBJECT
public:
    explicit QQnxCamera(QCamera *parent);

    bool isActive() const override;
    void setActive(bool active) override;

    void setCamera(const QCameraDevice &camera) override;

//    bool setCameraFormat(const QCameraFormat &/*format*/) override;

    void setCaptureSession(QPlatformMediaCaptureSession *session) override;

    bool isFocusModeSupported(QCamera::FocusMode mode) const override;
    void setFocusMode(QCamera::FocusMode mode) override;

    void setCustomFocusPoint(const QPointF &point) override;

//    void setFocusDistance(float) override;

//    // smaller 0: zoom instantly, rate in power-of-two/sec
    void zoomTo(float /*newZoomFactor*/, float /*rate*/ = -1.) override;

//    void setExposureCompensation(float) override;
//    int isoSensitivity() const override;
//    void setManualIsoSensitivity(int) override;
//    void setManualExposureTime(float) override;
//    float exposureTime() const override;

    bool isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const override;
    void setWhiteBalanceMode(QCamera::WhiteBalanceMode /*mode*/) override;
    void setColorTemperature(int /*temperature*/) override;

    camera_handle_t handle() const;

    QList<camera_vfmode_t> supportedVfModes() const;
    QList<camera_res_t> supportedVfResolutions() const;

private:
    void updateCameraFeatures();
    void setColorTemperatureInternal(unsigned temp);

    template <typename T>
    using QueryFuncPtr = camera_error_t (*)(camera_handle_t, uint32_t, uint32_t *, T *);

    template <typename T>
    QList<T> queryValues(QueryFuncPtr<T> func) const;

    QQnxMediaCaptureSession *m_session;

    QCameraDevice m_camera;
    camera_unit_t m_cameraUnit = CAMERA_UNIT_NONE;

    CameraHandle m_handle;

    uint minZoom = 1;
    uint maxZoom = 1;
    mutable bool whiteBalanceModesChecked = false;
    mutable bool continuousColorTemperatureSupported = false;
    mutable int minColorTemperature = 0;
    mutable int maxColorTemperature = 0;
    mutable QList<unsigned> manualColorTemperatureValues;
};

QT_END_NAMESPACE

#endif

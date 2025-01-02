// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQNXCAMERAHANDLE_P_H
#define QQNXCAMERAHANDLE_P_H

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

#include <camera/camera_api.h>

#include <utility>

class QQnxCameraHandle
{
public:
    QQnxCameraHandle() = default;

    explicit QQnxCameraHandle(camera_handle_t h)
        : m_handle (h) {}

    explicit QQnxCameraHandle(QQnxCameraHandle &&other)
        : m_handle(other.m_handle)
        , m_lastError(other.m_lastError)
    {
        other = QQnxCameraHandle();
    }

    QQnxCameraHandle(const QQnxCameraHandle&) = delete;

    QQnxCameraHandle& operator=(QQnxCameraHandle&& other)
    {
        m_handle = other.m_handle;
        m_lastError = other.m_lastError;

        other = QQnxCameraHandle();

        return *this;
    }

    ~QQnxCameraHandle()
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
            return true;

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

#endif

/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

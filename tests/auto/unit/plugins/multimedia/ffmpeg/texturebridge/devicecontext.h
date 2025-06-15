// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QtCore/private/qcomptr_p.h>
#include <QtCore/private/qexpected_p.h>
#include <QtGui/qcolor.h>
#include <QtCore/qsize.h>
#include <vector>

#include <d3d11_1.h>

template <typename T>
using ComResult = q23::expected<T, HRESULT>;

struct DeviceContext
{
    ComPtr<ID3D11Device1> device;
    ComPtr<ID3D11DeviceContext> context;

    ComResult<QColor> getFirstPixelColor(const ComPtr<ID3D11Texture2D> &texture) const;
    ComResult<ComPtr<ID3D11Texture2D>> createTextureArray(QSize size,
                                                          const std::vector<QColor> &colors) const;

private:
    // Function to set the pixels of an ID3D11Texture2D texture to red
    HRESULT fillTextureWithColors(const ComPtr<ID3D11Texture2D> &texture,
                                  const std::vector<QColor> &colors) const;
};

ComResult<DeviceContext> createDeviceContext();

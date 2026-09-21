// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWINDOWSD3D11TESTDEVICECONTEXT_P_H
#define QWINDOWSD3D11TESTDEVICECONTEXT_P_H

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

#include <QtCore/qbytearray.h>
#include <QtCore/qbytearrayview.h>
#include <QtCore/qsize.h>
#include <QtCore/private/qcomptr_p.h>
#include <QtCore/private/qexpected_p.h>
#include <QtGui/qcolor.h>

#include <vector>

#include <d3d11_1.h>

class QRhi;

template <typename T>
using ComResult = q23::expected<T, HRESULT>;

// Shared D3D11 device fixture for Windows-only multimedia unit tests.
struct QWindowsD3D11TestDeviceContext
{
    ComPtr<ID3D11Device1> device;
    ComPtr<ID3D11DeviceContext> context;

    ComResult<QColor> getFirstPixelColor(const ComPtr<ID3D11Texture2D> &texture) const;
    ComResult<ComPtr<ID3D11Texture2D>> createTextureArray(QSize size,
                                                          const std::vector<QColor> &colors) const;

    // Creates an uninitialized single-plane NV12 texture, e.g. for structural (plane
    // count/size/format) tests that don't need to verify pixel contents.
    ComResult<ComPtr<ID3D11Texture2D>> createNV12Texture(QSize size) const;

    // Fills an NV12 texture's luma plane with lumaValue and its interleaved chroma plane with
    // (chromaUValue, chromaVValue), e.g. for pixel round-trip tests. If texture is a texture
    // array (e.g. a pooled decoder surface), arraySlice selects which slice to fill.
    HRESULT fillNV12Texture(const ComPtr<ID3D11Texture2D> &texture, QSize size, quint8 lumaValue,
                            quint8 chromaUValue, quint8 chromaVValue, UINT arraySlice = 0) const;

    // Same as createNV12Texture, but for a 10-bit P010 surface.
    ComResult<ComPtr<ID3D11Texture2D>> createP010Texture(QSize size) const;

    // Same as fillNV12Texture, but for a 10-bit P010 surface: each 16-bit sample holds its value
    // left-shifted into the high bits (FFmpeg's P010 convention).
    HRESULT fillP010Texture(const ComPtr<ID3D11Texture2D> &texture, QSize size, quint16 lumaValue,
                            quint16 chromaUValue, quint16 chromaVValue, UINT arraySlice = 0) const;

    // Creates an uninitialized single-plane texture in the given (non-planar) format, e.g. for
    // structural tests of a packed hw pixel format.
    ComResult<ComPtr<ID3D11Texture2D>> createSinglePlaneTexture(QSize size,
                                                                DXGI_FORMAT format) const;

    // Fills a single-plane texture uniformly by repeating texel across every pixel.
    HRESULT fillSinglePlaneTexture(const ComPtr<ID3D11Texture2D> &texture, QSize size,
                                   QByteArrayView texel, UINT arraySlice = 0) const;

    // Reads back "height" rows of "width" texels ("texelSize" bytes each), tightly packed (row
    // padding from the texture's native row pitch is stripped), starting "rowOffset" rows into
    // the given array slice. Suitable for verifying any plane of any pixel format, e.g. the
    // second (chroma) plane of a planar format by passing rowOffset = the first plane's height.
    ComResult<QByteArray> readPlaneRows(const ComPtr<ID3D11Texture2D> &texture, int rowOffset,
                                        int width, int height, int texelSize,
                                        UINT arraySlice = 0) const;

private:
    // Function to set the pixels of an ID3D11Texture2D texture to red
    HRESULT fillTextureWithColors(const ComPtr<ID3D11Texture2D> &texture,
                                  const std::vector<QColor> &colors) const;
};

// Creates a device context using a hardware D3D11 adapter.
ComResult<QWindowsD3D11TestDeviceContext> createD3D11TestDeviceContext();

// Creates a device context using the software (WARP) D3D11 adapter, e.g. to test behavior
// with a QRhi that reports QRhiDriverInfo::CpuDevice.
ComResult<QWindowsD3D11TestDeviceContext> createWarpD3D11TestDeviceContext();

// Get QWindowsD3D11TestDeviceContext from QRhi instance
ComResult<QWindowsD3D11TestDeviceContext> wrapRhiD3D11Device(QRhi &rhi);

#endif // QWINDOWSD3D11TESTDEVICECONTEXT_P_H

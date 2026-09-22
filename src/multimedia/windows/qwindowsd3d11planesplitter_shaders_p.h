// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QWINDOWSD3D11PLANESPLITTER_SHADERS_P_H
#define QWINDOWSD3D11PLANESPLITTER_SHADERS_P_H

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

#include <QtCore/qglobal.h>

#include <d3d11.h>

// Precompiled DXBC bytecode for the two tiny shaders used by QWindowsD3D11PlaneSplitter to copy
// one plane-view of a multi-planar D3D11 texture into its own single-plane render target. Using
// precompiled bytecode (rather than calling D3DCompile at runtime) avoids adding a runtime
// dependency on d3dcompiler_47.dll for this module.
//
// To regenerate: copy each block below verbatim into its file and run the fxc command underneath
// it (Windows SDK's fxc.exe). No stripping or unescaping needed.

// --- plane_split_vs.hlsl ---
/*
 // Fullscreen triangle without any vertex/index buffer or input layout.
 // Draw(3, 0) with a null input layout feeds ids 0, 1, 2 into SV_VertexID:
 //   id 0 -> uv (0,0) -> pos (-1, +1, 0, 1)  (top-left)
 //   id 1 -> uv (2,0) -> pos (+3, +1, 0, 1)  (top-right, off-screen)
 //   id 2 -> uv (0,2) -> pos (-1, -3, 0, 1)  (bottom-left, off-screen)
 // The oversized triangle covers every viewport pixel; the rasterizer clips it.
 // Y is flipped (uv.y * -2 + 1) for D3D texture coordinates.
 void main(in uint id : SV_VertexID, out float4 pos : SV_Position)
 {
     float2 uv = float2((id << 1) & 2, id & 2);
     pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
 }
*/
//   fxc /T vs_4_0 /E main /Fh vs.h /Vn g_planeSplitVS plane_split_vs.hlsl

// --- plane_split_ps.hlsl ---
/*
 // Per-plane texel copy. The SRV is a format-override Texture2DArray view of one
 // slice (FirstArraySlice) of the source texture, so Load reads the plane at its
 // native resolution with no sampler or filtering.
 // SV_Position is already in integer render-target pixel coordinates.
 Texture2DArray<float4> g_planeView : register(t0);

 float4 main(float4 pos : SV_Position) : SV_Target0
 {
     return g_planeView.Load(int4(int2(pos.xy), 0, 0));
 }
*/
//   fxc /T ps_4_0 /E main /Fh ps.h /Vn g_planeSplitPS plane_split_ps.hlsl

// HLSL texture-fetch rule relied upon: the missing components of the generic float4 load
// default to 0 (alpha to 1), so the same bytecode serves both the single-channel luma plane
// and the two-channel chroma plane, and both 8- and 16-bit formats — only as many components
// as the bound render target actually stores are written.

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

// clang-format off
constexpr BYTE g_planeSplitVS[] =
{
     68,  88,  66,  67,  53,  98,
    221, 212, 186, 157, 245,  96,
     19,  95, 165,  87, 229, 177,
    102, 204,   1,   0,   0,   0,
    100,   2,   0,   0,   5,   0,
      0,   0,  52,   0,   0,   0,
    128,   0,   0,   0, 180,   0,
      0,   0, 232,   0,   0,   0,
    232,   1,   0,   0,  82,  68,
     69,  70,  68,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
     28,   0,   0,   0,   0,   4,
    254, 255,   0,   1,   0,   0,
     28,   0,   0,   0,  77, 105,
     99, 114, 111, 115, 111, 102,
    116,  32,  40,  82,  41,  32,
     72,  76,  83,  76,  32,  83,
    104,  97, 100, 101, 114,  32,
     67, 111, 109, 112, 105, 108,
    101, 114,  32,  49,  48,  46,
     49,   0,  73,  83,  71,  78,
     44,   0,   0,   0,   1,   0,
      0,   0,   8,   0,   0,   0,
     32,   0,   0,   0,   0,   0,
      0,   0,   6,   0,   0,   0,
      1,   0,   0,   0,   0,   0,
      0,   0,   1,   1,   0,   0,
     83,  86,  95,  86, 101, 114,
    116, 101, 120,  73,  68,   0,
     79,  83,  71,  78,  44,   0,
      0,   0,   1,   0,   0,   0,
      8,   0,   0,   0,  32,   0,
      0,   0,   0,   0,   0,   0,
      1,   0,   0,   0,   3,   0,
      0,   0,   0,   0,   0,   0,
     15,   0,   0,   0,  83,  86,
     95,  80, 111, 115, 105, 116,
    105, 111, 110,   0,  83,  72,
     68,  82, 248,   0,   0,   0,
     64,   0,   1,   0,  62,   0,
      0,   0,  96,   0,   0,   4,
     18,  16,  16,   0,   0,   0,
      0,   0,   6,   0,   0,   0,
    103,   0,   0,   4, 242,  32,
     16,   0,   0,   0,   0,   0,
      1,   0,   0,   0, 104,   0,
      0,   2,   1,   0,   0,   0,
     41,   0,   0,   7,  18,   0,
     16,   0,   0,   0,   0,   0,
     10,  16,  16,   0,   0,   0,
      0,   0,   1,  64,   0,   0,
      1,   0,   0,   0,   1,   0,
      0,   7,  18,   0,  16,   0,
      0,   0,   0,   0,  10,   0,
     16,   0,   0,   0,   0,   0,
      1,  64,   0,   0,   2,   0,
      0,   0,   1,   0,   0,   7,
     66,   0,  16,   0,   0,   0,
      0,   0,  10,  16,  16,   0,
      0,   0,   0,   0,   1,  64,
      0,   0,   2,   0,   0,   0,
     86,   0,   0,   5,  50,   0,
     16,   0,   0,   0,   0,   0,
    134,   0,  16,   0,   0,   0,
      0,   0,  50,   0,   0,  15,
     50,  32,  16,   0,   0,   0,
      0,   0,  70,   0,  16,   0,
      0,   0,   0,   0,   2,  64,
      0,   0,   0,   0,   0,  64,
      0,   0,   0, 192,   0,   0,
      0,   0,   0,   0,   0,   0,
      2,  64,   0,   0,   0,   0,
    128, 191,   0,   0, 128,  63,
      0,   0,   0,   0,   0,   0,
      0,   0,  54,   0,   0,   8,
    194,  32,  16,   0,   0,   0,
      0,   0,   2,  64,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0, 128,  63,  62,   0,
      0,   1,  83,  84,  65,  84,
    116,   0,   0,   0,   7,   0,
      0,   0,   1,   0,   0,   0,
      0,   0,   0,   0,   2,   0,
      0,   0,   1,   0,   0,   0,
      1,   0,   0,   0,   2,   0,
      0,   0,   1,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   1,   0,   0,   0,
      0,   0,   0,   0,   1,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
};

constexpr BYTE g_planeSplitPS[] =
{
     68,  88,  66,  67,  62, 209,
    231,  68, 159, 106,   7,  25,
     79,  72, 170,  49, 182, 154,
      2, 249,   1,   0,   0,   0,
     40,   2,   0,   0,   5,   0,
      0,   0,  52,   0,   0,   0,
    172,   0,   0,   0, 224,   0,
      0,   0,  20,   1,   0,   0,
    172,   1,   0,   0,  82,  68,
     69,  70, 112,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   1,   0,   0,   0,
     28,   0,   0,   0,   0,   4,
    255, 255,   0,   1,   0,   0,
     72,   0,   0,   0,  60,   0,
      0,   0,   2,   0,   0,   0,
      5,   0,   0,   0,   5,   0,
      0,   0, 255, 255, 255, 255,
      0,   0,   0,   0,   1,   0,
      0,   0,  13,   0,   0,   0,
    103,  95, 112, 108,  97, 110,
    101,  86, 105, 101, 119,   0,
     77, 105,  99, 114, 111, 115,
    111, 102, 116,  32,  40,  82,
     41,  32,  72,  76,  83,  76,
     32,  83, 104,  97, 100, 101,
    114,  32,  67, 111, 109, 112,
    105, 108, 101, 114,  32,  49,
     48,  46,  49,   0,  73,  83,
     71,  78,  44,   0,   0,   0,
      1,   0,   0,   0,   8,   0,
      0,   0,  32,   0,   0,   0,
      0,   0,   0,   0,   1,   0,
      0,   0,   3,   0,   0,   0,
      0,   0,   0,   0,  15,   3,
      0,   0,  83,  86,  95,  80,
    111, 115, 105, 116, 105, 111,
    110,   0,  79,  83,  71,  78,
     44,   0,   0,   0,   1,   0,
      0,   0,   8,   0,   0,   0,
     32,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      3,   0,   0,   0,   0,   0,
      0,   0,  15,   0,   0,   0,
     83,  86,  95,  84,  97, 114,
    103, 101, 116,   0, 171, 171,
     83,  72,  68,  82, 144,   0,
      0,   0,  64,   0,   0,   0,
     36,   0,   0,   0,  88,  64,
      0,   4,   0, 112,  16,   0,
      0,   0,   0,   0,  85,  85,
      0,   0, 100,  32,   0,   4,
     50,  16,  16,   0,   0,   0,
      0,   0,   1,   0,   0,   0,
    101,   0,   0,   3, 242,  32,
     16,   0,   0,   0,   0,   0,
    104,   0,   0,   2,   1,   0,
      0,   0,  27,   0,   0,   5,
     50,   0,  16,   0,   0,   0,
      0,   0,  70,  16,  16,   0,
      0,   0,   0,   0,  54,   0,
      0,   8, 194,   0,  16,   0,
      0,   0,   0,   0,   2,  64,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
     45,   0,   0,   7, 242,  32,
     16,   0,   0,   0,   0,   0,
     70,  14,  16,   0,   0,   0,
      0,   0,  70, 126,  16,   0,
      0,   0,   0,   0,  62,   0,
      0,   1,  83,  84,  65,  84,
    116,   0,   0,   0,   4,   0,
      0,   0,   1,   0,   0,   0,
      0,   0,   0,   0,   2,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   1,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   1,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   1,   0,   0,   0,
      0,   0,   0,   0,   1,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
};
// clang-format on

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QWINDOWSD3D11PLANESPLITTER_SHADERS_P_H

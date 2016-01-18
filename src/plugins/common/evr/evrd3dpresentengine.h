/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef EVRD3DPRESENTENGINE_H
#define EVRD3DPRESENTENGINE_H

#include <QMutex>
#include <QVideoSurfaceFormat>

#include <d3d9.h>

#if defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_DYNAMIC)
# include <EGL/egl.h>
# define MAYBE_ANGLE
#endif

QT_BEGIN_NAMESPACE
class QAbstractVideoSurface;
QT_END_NAMESPACE

struct IDirect3D9Ex;
struct IDirect3DDevice9Ex;
struct IDirect3DDeviceManager9;
struct IDirect3DSurface9;
struct IDirect3DTexture9;
struct IMFSample;
struct IMFMediaType;

// Randomly generated GUIDs
static const GUID MFSamplePresenter_SampleCounter =
{ 0xb0bb83cc, 0xf10f, 0x4e2e, { 0xaa, 0x2b, 0x29, 0xea, 0x5e, 0x92, 0xef, 0x85 } };

QT_USE_NAMESPACE

#ifdef MAYBE_ANGLE

class OpenGLResources;

class EGLWrapper
{
public:
    EGLWrapper();

    __eglMustCastToProperFunctionPointerType getProcAddress(const char *procname);
    EGLSurface createPbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
    EGLBoolean destroySurface(EGLDisplay dpy, EGLSurface surface);
    EGLBoolean bindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
    EGLBoolean releaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);

private:
    typedef __eglMustCastToProperFunctionPointerType (EGLAPIENTRYP EglGetProcAddress)(const char *procname);
    typedef EGLSurface (EGLAPIENTRYP EglCreatePbufferSurface)(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
    typedef EGLBoolean (EGLAPIENTRYP EglDestroySurface)(EGLDisplay dpy, EGLSurface surface);
    typedef EGLBoolean (EGLAPIENTRYP EglBindTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
    typedef EGLBoolean (EGLAPIENTRYP EglReleaseTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);

    EglGetProcAddress m_eglGetProcAddress;
    EglCreatePbufferSurface m_eglCreatePbufferSurface;
    EglDestroySurface m_eglDestroySurface;
    EglBindTexImage m_eglBindTexImage;
    EglReleaseTexImage m_eglReleaseTexImage;
};

#endif // MAYBE_ANGLE

class D3DPresentEngine
{
public:
    enum Hint
    {
        RenderToTexture
    };

    D3DPresentEngine();
    virtual ~D3DPresentEngine();

    bool isValid() const;
    void setHint(Hint hint, bool enable = true);

    HRESULT getService(REFGUID guidService, REFIID riid, void** ppv);
    HRESULT checkFormat(D3DFORMAT format);
    UINT refreshRate() const { return m_displayMode.RefreshRate; }

    bool supportsTextureRendering() const;
    bool isTextureRenderingEnabled() const { return m_useTextureRendering; }

    HRESULT createVideoSamples(IMFMediaType *format, QList<IMFSample*>& videoSampleQueue);
    QVideoSurfaceFormat videoSurfaceFormat() const { return m_surfaceFormat; }
    QVideoFrame makeVideoFrame(IMFSample* sample);

    void releaseResources();

private:
    HRESULT initializeD3D();
    HRESULT createD3DDevice();


    UINT m_deviceResetToken;
    D3DDISPLAYMODE m_displayMode;

    IDirect3D9Ex *m_D3D9;
    IDirect3DDevice9Ex *m_device;
    IDirect3DDeviceManager9 *m_deviceManager;

    QVideoSurfaceFormat m_surfaceFormat;

    bool m_useTextureRendering;

#ifdef MAYBE_ANGLE
    bool createRenderTexture();
    bool updateTexture(IDirect3DSurface9 *src);

    OpenGLResources *m_glResources;
    IDirect3DTexture9 *m_texture;
#endif

    friend class IMFSampleVideoBuffer;
};

#endif // EVRD3DPRESENTENGINE_H

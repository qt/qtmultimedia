// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef EVRD3DPRESENTENGINE_H
#define EVRD3DPRESENTENGINE_H

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

#include <QMutex>
#include <QSize>
#include <QVideoFrameFormat>
#include <QtCore/private/qcomptr_p.h>
#include <qpointer.h>

#include <d3d9.h>

struct IDirect3D9Ex;
struct IDirect3DDevice9Ex;
struct IDirect3DDeviceManager9;
struct IDirect3DSurface9;
struct IDirect3DTexture9;
struct IMFSample;
struct IMFMediaType;

QT_BEGIN_NAMESPACE
class QVideoFrame;
class QVideoSink;
QT_END_NAMESPACE

// Randomly generated GUIDs
static const GUID MFSamplePresenter_SampleCounter =
{ 0xb0bb83cc, 0xf10f, 0x4e2e, { 0xaa, 0x2b, 0x29, 0xea, 0x5e, 0x92, 0xef, 0x85 } };

#if QT_CONFIG(opengl)
#    include <qopengl.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef MAYBE_ANGLE

class OpenGLResources;

class EGLWrapper
{
    Q_DISABLE_COPY(EGLWrapper)
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

#if QT_CONFIG(opengl)

struct WglNvDxInterop {
    HANDLE (WINAPI* wglDXOpenDeviceNV) (void* dxDevice);
    BOOL (WINAPI* wglDXCloseDeviceNV) (HANDLE hDevice);
    HANDLE (WINAPI* wglDXRegisterObjectNV) (HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access);
    BOOL (WINAPI* wglDXSetResourceShareHandleNV) (void *dxResource, HANDLE shareHandle);
    BOOL (WINAPI* wglDXLockObjectsNV) (HANDLE hDevice, GLint count, HANDLE *hObjects);
    BOOL (WINAPI* wglDXUnlockObjectsNV) (HANDLE hDevice, GLint count, HANDLE *hObjects);
    BOOL (WINAPI* wglDXUnregisterObjectNV) (HANDLE hDevice, HANDLE hObject);

    static const int WGL_ACCESS_READ_ONLY_NV = 0;
};

#endif

class D3DPresentEngine
{
    Q_DISABLE_COPY(D3DPresentEngine)
public:
    D3DPresentEngine(QVideoSink *sink);
    virtual ~D3DPresentEngine();

    bool isValid() const;

    HRESULT getService(REFGUID guidService, REFIID riid, void** ppv);
    HRESULT checkFormat(D3DFORMAT format);
    UINT refreshRate() const { return m_displayMode.RefreshRate; }

    HRESULT createVideoSamples(IMFMediaType *format, QList<ComPtr<IMFSample>> &videoSampleQueue,
                               QSize frameSize);
    QVideoFrameFormat videoSurfaceFormat() const { return m_surfaceFormat; }
    QVideoFrame makeVideoFrame(const ComPtr<IMFSample> &sample, QtVideo::Rotation rotation);

    void releaseResources();
    void setSink(QVideoSink *sink);

private:
    static const int PRESENTER_BUFFER_COUNT = 3;

    HRESULT initializeD3D();
    HRESULT createD3DDevice();

    std::pair<IMFSample *, HANDLE> m_sampleTextureHandle[PRESENTER_BUFFER_COUNT] = {};

    UINT m_deviceResetToken;
    D3DDISPLAYMODE m_displayMode;

    ComPtr<IDirect3D9Ex> m_D3D9;
    ComPtr<IDirect3DDevice9Ex> m_device;
    ComPtr<IDirect3DDeviceManager9> m_devices;

    QVideoFrameFormat m_surfaceFormat;

    QPointer<QVideoSink> m_sink;
    bool m_useTextureRendering = false;
#if QT_CONFIG(opengl)
    WglNvDxInterop m_wglNvDxInterop;
#endif

#ifdef MAYBE_ANGLE
    unsigned int updateTexture(IDirect3DSurface9 *src);

    OpenGLResources *m_glResources;
    IDirect3DTexture9 *m_texture;
#endif

    friend class IMFSampleVideoBuffer;
};

QT_END_NAMESPACE

#endif // EVRD3DPRESENTENGINE_H

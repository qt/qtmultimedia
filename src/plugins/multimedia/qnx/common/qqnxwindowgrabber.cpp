// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxwindowgrabber_p.h"

#include <QAbstractEventDispatcher>
#include <QDebug>
#include <QGuiApplication>
#include <QImage>
#include <QThread>
#include <qpa/qplatformnativeinterface.h>

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <rhi/qrhi.h>

#include <cstring>

#include <EGL/egl.h>
#include <errno.h>

QT_BEGIN_NAMESPACE

static PFNEGLCREATEIMAGEKHRPROC s_eglCreateImageKHR;
static PFNEGLDESTROYIMAGEKHRPROC s_eglDestroyImageKHR;

class QQnxWindowGrabberImage
{
public:
    QQnxWindowGrabberImage();
    ~QQnxWindowGrabberImage();

    bool initialize(screen_context_t screenContext);

    QQnxWindowGrabber::BufferView getBuffer(screen_window_t window, const QSize &size);
    GLuint getTexture(screen_window_t window, const QSize &size);

private:
    bool grab(screen_window_t window);
    bool resize(const QSize &size);

    QSize m_size;
    screen_pixmap_t m_pixmap;
    screen_buffer_t m_pixmapBuffer;
    EGLImageKHR m_eglImage;
    GLuint m_glTexture;
    unsigned char *m_bufferAddress;
    int m_bufferStride;
};

QQnxWindowGrabber::QQnxWindowGrabber(QObject *parent)
    : QObject(parent),
      m_windowParent(nullptr),
      m_window(nullptr),
      m_screenContext(nullptr),
      m_rhi(nullptr),
      m_active(false),
      m_eglImageSupported(false),
      m_startPending(false)
{
    // grab the window frame with 60 frames per second
    m_timer.setInterval(1000/60);

    connect(&m_timer, &QTimer::timeout, this, &QQnxWindowGrabber::triggerUpdate);

    QCoreApplication::eventDispatcher()->installNativeEventFilter(this);

    // Use of EGL images can be disabled by setting QQNX_MM_DISABLE_EGLIMAGE_SUPPORT to something
    // non-zero.  This is probably useful only to test that this path still works since it results
    // in a high CPU load.
    if (!s_eglCreateImageKHR && qgetenv("QQNX_MM_DISABLE_EGLIMAGE_SUPPORT").toInt() == 0) {
        s_eglCreateImageKHR = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
        s_eglDestroyImageKHR = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    }

    QPlatformNativeInterface *const nativeInterface = QGuiApplication::platformNativeInterface();
    if (nativeInterface) {
        m_screenContext = static_cast<screen_context_t>(
            nativeInterface->nativeResourceForIntegration("screenContext"));
    }

    // Create a parent window for the window whose content will be grabbed. Since the
    // window is only a buffer conduit, the characteristics of the parent window are
    // irrelevant.  The contents of the window can be grabbed so long as the window
    // joins the parent window's group and the parent window is in this process.
    // Using the window that displays this content isn't possible because there's no
    // way to reliably retrieve it from this code or any calling code.
    screen_create_window(&m_windowParent, m_screenContext);
    screen_create_window_group(m_windowParent, nullptr);
}

QQnxWindowGrabber::~QQnxWindowGrabber()
{
    screen_destroy_window(m_windowParent);
    QCoreApplication::eventDispatcher()->removeNativeEventFilter(this);
}

void QQnxWindowGrabber::setFrameRate(int frameRate)
{
    m_timer.setInterval(1000/frameRate);
}

void QQnxWindowGrabber::setWindowId(const QByteArray &windowId)
{
    m_windowId = windowId;
}

void QQnxWindowGrabber::setRhi(QRhi *rhi)
{
    m_rhi = rhi;

    checkForEglImageExtension();
}

void QQnxWindowGrabber::start()
{
    if (m_active)
        return;

    if (!m_window) {
        m_startPending = true;
        return;
    }

    m_startPending = false;

    if (!m_screenContext)
        screen_get_window_property_pv(m_window, SCREEN_PROPERTY_CONTEXT, reinterpret_cast<void**>(&m_screenContext));

    m_timer.start();

    m_active = true;
}

void QQnxWindowGrabber::stop()
{
    if (!m_active)
        return;

    resetBuffers();

    m_timer.stop();

    m_active = false;
}

void QQnxWindowGrabber::pause()
{
    m_timer.stop();
}

void QQnxWindowGrabber::resume()
{
    if (!m_active)
        return;

    m_timer.start();
}

void QQnxWindowGrabber::forceUpdate()
{
    if (!m_active)
        return;

    triggerUpdate();
}

bool QQnxWindowGrabber::handleScreenEvent(screen_event_t screen_event)
{

    int eventType;
    if (screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TYPE, &eventType) != 0) {
        qWarning() << "QQnxWindowGrabber: Failed to query screen event type";
        return false;
    }

    if (eventType != SCREEN_EVENT_CREATE)
        return false;

    screen_window_t window = 0;
    if (screen_get_event_property_pv(screen_event, SCREEN_PROPERTY_WINDOW, (void**)&window) != 0) {
        qWarning() << "QQnxWindowGrabber: Failed to query window property";
        return false;
    }

    const int maxIdStrLength = 128;
    char idString[maxIdStrLength];
    if (screen_get_window_property_cv(window, SCREEN_PROPERTY_ID_STRING, maxIdStrLength, idString) != 0) {
        qWarning() << "QQnxWindowGrabber: Failed to query window ID string";
        return false;
    }

    // Grab windows that have a non-empty ID string and a matching window id to grab
    if (idString[0] != '\0' && m_windowId == idString) {
        m_window = window;

        if (m_startPending)
            start();
    }

    return false;
}

bool QQnxWindowGrabber::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *)
{
    if (eventType == "screen_event_t") {
        const screen_event_t event = static_cast<screen_event_t>(message);
        return handleScreenEvent(event);
    }

    return false;
}

QByteArray QQnxWindowGrabber::windowGroupId() const
{
    char groupName[256];
    memset(groupName, 0, sizeof(groupName));
    screen_get_window_property_cv(m_windowParent,
                                  SCREEN_PROPERTY_GROUP,
                                  sizeof(groupName) - 1,
                                  groupName);
    return QByteArray(groupName);
}

bool QQnxWindowGrabber::isEglImageSupported() const
{
    return m_eglImageSupported;
}

void QQnxWindowGrabber::checkForEglImageExtension()
{
    m_eglImageSupported = false;

    if (!m_rhi || m_rhi->backend() != QRhi::OpenGLES2)
        return;

    const EGLDisplay defaultDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    const char *vendor = eglQueryString(defaultDisplay, EGL_VENDOR);

    if (vendor && std::strstr(vendor, "VMWare"))
        return;

    const char *eglExtensions = eglQueryString(defaultDisplay, EGL_EXTENSIONS);

    if (!eglExtensions)
        return;

    m_eglImageSupported = std::strstr(eglExtensions, "EGL_KHR_image")
                          && s_eglCreateImageKHR
                          && s_eglDestroyImageKHR;
}

void QQnxWindowGrabber::triggerUpdate()
{
    int size[2] = { 0, 0 };

    const int result = screen_get_window_property_iv(m_window, SCREEN_PROPERTY_SOURCE_SIZE, size);

    if (result != 0) {
        resetBuffers();
        qWarning() << "QQnxWindowGrabber: cannot get window size:" << strerror(errno);
        return;
    }

    if (m_size.width() != size[0] || m_size.height() != size[1])
        m_size = QSize(size[0], size[1]);

    emit updateScene(m_size);
}

bool QQnxWindowGrabber::selectBuffer()
{
    // If we're using egl images we need to double buffer since the gpu may still be using the last
    // video frame.  If we're not, it doesn't matter since the data is immediately copied.
    if (isEglImageSupported())
        std::swap(m_frontBuffer, m_backBuffer);

    if (m_frontBuffer)
        return true;

    auto frontBuffer = std::make_unique<QQnxWindowGrabberImage>();

    if (!frontBuffer->initialize(m_screenContext))
        return false;

    m_frontBuffer = std::move(frontBuffer);

    return true;
}

int QQnxWindowGrabber::getNextTextureId()
{
    if (!selectBuffer())
        return 0;

    return m_frontBuffer->getTexture(m_window, m_size);
}

QQnxWindowGrabber::BufferView QQnxWindowGrabber::getNextBuffer()
{
    if (!selectBuffer())
        return {};

    return m_frontBuffer->getBuffer(m_window, m_size);
}

void QQnxWindowGrabber::resetBuffers()
{
    m_frontBuffer.reset();
    m_backBuffer.reset();
}

QQnxWindowGrabberImage::QQnxWindowGrabberImage()
    : m_pixmap(0),
    m_pixmapBuffer(0),
    m_eglImage(0),
    m_glTexture(0),
    m_bufferAddress(nullptr),
    m_bufferStride(0)
{
}

QQnxWindowGrabberImage::~QQnxWindowGrabberImage()
{
    if (m_glTexture)
        glDeleteTextures(1, &m_glTexture);
    if (m_eglImage)
        s_eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), m_eglImage);
    if (m_pixmap)
        screen_destroy_pixmap(m_pixmap);
}

bool QQnxWindowGrabberImage::initialize(screen_context_t screenContext)
{
    if (screen_create_pixmap(&m_pixmap, screenContext) != 0) {
        qWarning() << "QQnxWindowGrabber: cannot create pixmap:" << strerror(errno);
        return false;
    }
    const int usage = SCREEN_USAGE_WRITE | SCREEN_USAGE_READ | SCREEN_USAGE_NATIVE;
    screen_set_pixmap_property_iv(m_pixmap, SCREEN_PROPERTY_USAGE, &usage);

    // XXX as a matter of fact, the underlying buffer is BGRX8888 (according to
    // QNX, screen formats can be loose on the ARGB ordering) - as there is no
    // SCREEN_FORMAT_BGRX8888 constant, we use SCREEN_FORMAT_RGBX8888, which
    // carries the same depth and allows us to use the buffer.
    const int format = SCREEN_FORMAT_RGBX8888;
    screen_set_pixmap_property_iv(m_pixmap, SCREEN_PROPERTY_FORMAT, &format);

    return true;
}

bool QQnxWindowGrabberImage::resize(const QSize &newSize)
{
    if (m_pixmapBuffer) {
        screen_destroy_pixmap_buffer(m_pixmap);
        m_pixmapBuffer = 0;
        m_bufferAddress = 0;
        m_bufferStride = 0;
    }

    const int size[2] = { newSize.width(), newSize.height() };

    screen_set_pixmap_property_iv(m_pixmap, SCREEN_PROPERTY_BUFFER_SIZE, size);

    if (screen_create_pixmap_buffer(m_pixmap) == 0) {
        screen_get_pixmap_property_pv(m_pixmap, SCREEN_PROPERTY_RENDER_BUFFERS,
                                      reinterpret_cast<void**>(&m_pixmapBuffer));
        screen_get_buffer_property_pv(m_pixmapBuffer, SCREEN_PROPERTY_POINTER,
                                      reinterpret_cast<void**>(&m_bufferAddress));
        screen_get_buffer_property_iv(m_pixmapBuffer, SCREEN_PROPERTY_STRIDE, &m_bufferStride);
        m_size = newSize;

        return true;
    } else {
        m_size = QSize();
        return false;
    }
}

bool QQnxWindowGrabberImage::grab(screen_window_t window)
{
    const int rect[] = { 0, 0, m_size.width(), m_size.height() };
    return screen_read_window(window, m_pixmapBuffer, 1, rect, 0) == 0;
}

QQnxWindowGrabber::BufferView QQnxWindowGrabberImage::getBuffer(
        screen_window_t window, const QSize &size)
{
    if (size != m_size && !resize(size))
        return {};

    if (!m_bufferAddress || !grab(window))
        return {};

    return {
        .width = m_size.width(),
        .height = m_size.height(),
        .stride = m_bufferStride,
        .data = m_bufferAddress
    };
}

GLuint QQnxWindowGrabberImage::getTexture(screen_window_t window, const QSize &size)
{
    if (size != m_size) {
        // create a brand new texture to be the KHR image sibling, as
        // previously used textures cannot be reused with new KHR image
        // sources - note that glDeleteTextures handles nullptr gracefully
        glDeleteTextures(1, &m_glTexture);
        glGenTextures(1, &m_glTexture);

        glBindTexture(GL_TEXTURE_2D, m_glTexture);
        if (m_eglImage) {
            glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, 0);
            s_eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), m_eglImage);
            m_eglImage = 0;
        }
        if (!resize(size))
            return 0;
        m_eglImage = s_eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), EGL_NO_CONTEXT,
                                         EGL_NATIVE_PIXMAP_KHR, m_pixmap, 0);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
    }

    if (!m_pixmap || !grab(window))
        return 0;

    return m_glTexture;
}

QT_END_NAMESPACE

#include "moc_qqnxwindowgrabber_p.cpp"

// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidscreencapture_p.h"

#include <QtCore/private/qjnihelpers_p.h>
#include <QReadWriteLock>
#include <qffmpegvideobuffer_p.h>
#include <private/qvideoframe_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtScreenGrabber,
                    "org/qtproject/qt/android/multimedia/QtScreenGrabber")
Q_DECLARE_JNI_CLASS(QtScreenCaptureService,
                    "org/qtproject/qt/android/multimedia/QtScreenCaptureService")
Q_DECLARE_JNI_CLASS(Size, "android/util/Size")

typedef QMap<int, QAndroidScreenCapture *> QAndroidScreenCaptureMap;
Q_GLOBAL_STATIC(QAndroidScreenCaptureMap, g_qsurfaceCaptures)
Q_GLOBAL_STATIC(QReadWriteLock, rwLock)


namespace {
QAtomicInteger<int> idCounter = 0;
constexpr int REQUEST_CODE_MEDIA_PROJECTION = 24680; // Arbitrary
constexpr int RESULT_CANCEL = 0;
constexpr int RESULT_OK = -1;
}

class QAndroidScreenCapture::Grabber : public QtAndroidPrivate::ActivityResultListener
{
public:
    Grabber(int surfaceCaptureID)
        : m_surfaceCaptureId(surfaceCaptureID)
        , m_activityRequestCode(REQUEST_CODE_MEDIA_PROJECTION + surfaceCaptureID)
    {
        using namespace QtJniTypes;
        const auto sizeObj = QtScreenGrabber::callStaticMethod<Size>(
                                            "getScreenCaptureSize", QtAndroidPrivate::activity());
        const QSize size = QSize(sizeObj.callMethod<int>("getWidth"),
                                 sizeObj.callMethod<int>("getHeight"));
        m_format = QVideoFrameFormat(size, QVideoFrameFormat::Format_RGBA8888);

        if (m_format.frameHeight() > 0 && m_format.frameWidth() > 0) {
            QtAndroidPrivate::registerActivityResultListener(this);
            m_jniGrabber = QtScreenGrabber(QtAndroidPrivate::activity(), m_activityRequestCode);
        } else {
            updateError(QStringLiteral("Invalid Screen size: %1x%2. Screen capture not started")
                            .arg(m_format.frameHeight())
                            .arg(m_format.frameWidth()));
        }
    }

    bool handleActivityResult(jint requestCode, jint resultCode, jobject data) override
    {
        if (requestCode != m_activityRequestCode || m_jniGrabber == nullptr)
            return false;

        if (resultCode == RESULT_OK) {
            const QtJniTypes::Intent intent(data);
            const bool screenCaptureServiceStarted = m_jniGrabber.callMethod<bool>(
                                                        "startScreenCaptureService",
                                                        resultCode,
                                                        m_surfaceCaptureId,
                                                        m_format.frameWidth(),
                                                        m_format.frameHeight(),
                                                        intent);
            if (!screenCaptureServiceStarted)
                updateError(QStringLiteral("Cannot start screen capture service"));
        } else if (resultCode == RESULT_CANCEL) {
            updateError(QStringLiteral("Screen capture canceled"));
        }
        return true;
    }

    ~Grabber()
    {
        QtAndroidPrivate::unregisterActivityResultListener(this);
        m_jniGrabber.callMethod<bool>("stopScreenCaptureService");
    }

    QVideoFrameFormat format() const { return m_format; }

private:
    void updateError(const QString &errorString)
    {
        QWriteLocker locker(rwLock);
        if (g_qsurfaceCaptures->contains(m_surfaceCaptureId))
            QMetaObject::invokeMethod(g_qsurfaceCaptures->value(m_surfaceCaptureId),
                                      &QPlatformSurfaceCapture::updateError,
                                      Qt::QueuedConnection,
                                      QPlatformSurfaceCapture::InternalError,
                                      errorString);
    }

    QtJniTypes::QtScreenGrabber m_jniGrabber;
    const int m_surfaceCaptureId;
    const int m_activityRequestCode;
    QVideoFrameFormat m_format;
};

QAndroidScreenCapture::QAndroidScreenCapture()
    : QPlatformSurfaceCapture(ScreenSource{})
      , m_id(idCounter.fetchAndAddRelaxed(1))
{
    QWriteLocker locker(rwLock);
    g_qsurfaceCaptures->insert(m_id, this);

}

QAndroidScreenCapture::~QAndroidScreenCapture()
{
    QWriteLocker locker(rwLock);
    g_qsurfaceCaptures->remove(m_id);
}

QVideoFrameFormat QAndroidScreenCapture::frameFormat() const
{
    return m_grabber ? m_grabber->format() : QVideoFrameFormat();
}

bool QAndroidScreenCapture::setActiveInternal(bool active)
{
    if (active == static_cast<bool>(m_grabber))
        return true;

    if (m_grabber)
        m_grabber.reset();
    else
        m_grabber = std::make_unique<Grabber>(m_id);

    return static_cast<bool>(m_grabber) == active;
}

static void deleteFrame(void *opaque, uint8_t *data)
{
    Q_UNUSED(data);
    auto frame = reinterpret_cast<QAndroidVideoFrameBuffer *>(opaque);
    if (frame)
        delete frame;
}

void QAndroidScreenCapture::onNewFrameReceived(QtJniTypes::AndroidImage image)
{
    auto androidFrame = std::make_unique<QAndroidVideoFrameBuffer>(image);
    if (!androidFrame->isParsed()) {
        updateError(QPlatformSurfaceCapture::InternalError,
                    QStringLiteral("Cannot parse screen frame"));
        return;
    }
    if (!isActive()) {
        return;
    }
    auto avframe = QFFmpeg::makeAVFrame();
    avframe->width = androidFrame->size().width();
    avframe->height = androidFrame->size().height();
    avframe->format = QFFmpegVideoBuffer::toAVPixelFormat(androidFrame->format());
    avframe->pts = androidFrame->timestamp();
    avframe->linesize[0] = androidFrame->plane(0).rowStride;
    avframe->data[0] = (uint8_t*)androidFrame->plane(0).buf.constData();
    avframe->data[1] = nullptr;
    avframe->buf[0] = nullptr;
    avframe->extended_data = avframe->data;
    QVideoFrameFormat format(androidFrame->size(), androidFrame->format());
    avframe->opaque_ref = av_buffer_create(NULL, 1, deleteFrame, androidFrame.release(), 0);

    QVideoFrame videoFrame = QVideoFramePrivate::createFrame(
            std::make_unique<QFFmpegVideoBuffer>(std::move(avframe)), format);

    emit newVideoFrame(videoFrame);
}

static void onScreenFrameAvailable(JNIEnv *env, jobject obj, QtJniTypes::AndroidImage image, jint id)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    QReadLocker locker(rwLock);
    if (g_qsurfaceCaptures->contains(id))
        g_qsurfaceCaptures->value(id)->onNewFrameReceived(image);
    else
        image.callMethod<void>("close");
}
Q_DECLARE_JNI_NATIVE_METHOD(onScreenFrameAvailable)

static void onErrorUpdate(JNIEnv *env, jobject obj, QString errorString, jint id)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    QReadLocker locker(rwLock);
    if (g_qsurfaceCaptures->contains(id)) {
        QMetaObject::invokeMethod(g_qsurfaceCaptures->value(id),
                                  &QPlatformSurfaceCapture::updateError,
                                  Qt::QueuedConnection,
                                  QPlatformSurfaceCapture::InternalError,
                                  errorString);
    }
}
Q_DECLARE_JNI_NATIVE_METHOD(onErrorUpdate)


bool QAndroidScreenCapture::registerNativeMethods()
{
    using namespace QtJniTypes;
    static const bool registered = []() {
        return QtScreenCaptureService::registerNativeMethods(
                { Q_JNI_NATIVE_METHOD(onScreenFrameAvailable),
                  Q_JNI_NATIVE_METHOD(onErrorUpdate)});
    }();
    return registered;
}

QT_END_NAMESPACE

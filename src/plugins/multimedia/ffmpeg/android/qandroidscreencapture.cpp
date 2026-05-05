// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qandroidscreencapture_p.h>

#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/qreadwritelock.h>

#include <QtFFmpegMediaPluginImpl/private/qandroidvideoframefactory_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegsurfacecapturegrabber_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtScreenGrabber,
                    "org/qtproject/qt/android/multimedia/qffmpeg/QtScreenGrabber")
Q_DECLARE_JNI_CLASS(QtScreenCaptureService,
                    "org/qtproject/qt/android/multimedia/qffmpeg/QtScreenCaptureService")
Q_DECLARE_JNI_CLASS(Size, "android/util/Size")


namespace {
QAtomicInteger<int> idCounter = 0;
constexpr int REQUEST_CODE_MEDIA_PROJECTION = 24680; // Arbitrary
constexpr int RESULT_CANCEL = 0;
constexpr int RESULT_OK = -1;

}

class QAndroidScreenCapture::Grabber : public QtAndroidPrivate::ActivityResultListener,
                                       public QFFmpegSurfaceCaptureGrabber
{
public:
    Grabber(QAndroidScreenCapture & screenCapture)
        : m_activityRequestCode(REQUEST_CODE_MEDIA_PROJECTION + idCounter.fetchAndAddRelaxed(1))
        , m_screenCapture(screenCapture)
    {
        injectContextToGrabbingThread(&m_resourceContext);

        addFrameCallback(&screenCapture, &QAndroidScreenCapture::newVideoFrame);

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

        setFrameRate(screenCapture.frameRate().value_or(DefaultScreenCaptureFrameRate));
        m_format.setStreamFrameRate(frameRate());
    }

    QVideoFrame grabFrame() override
    {
        return m_resourceContext.latestFrame();
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
                                                        reinterpret_cast<jlong>(&m_screenCapture),
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

    ~Grabber() override
    {
        stop();
        QtAndroidPrivate::unregisterActivityResultListener(this);
        m_jniGrabber.callMethod<bool>("stopScreenCaptureService");
    }

    QVideoFrameFormat format() const { return m_format; }

    void onNewFrameReceived(QtJniTypes::Image image)
    {
        QMetaObject::invokeMethod(&m_resourceContext, &ResourceContext::updateLatestImageRef,
                                  image);
    }

private:
    void updateError(const QString &errorString)
    {
        QMetaObject::invokeMethod(&m_screenCapture,
                                  &QPlatformSurfaceCapture::updateError,
                                  Qt::QueuedConnection,
                                  QPlatformSurfaceCapture::Error::InternalError,
                                  errorString);
    }

    QtJniTypes::QtScreenGrabber m_jniGrabber;
    const int m_activityRequestCode;
    QAndroidScreenCapture & m_screenCapture;
    QVideoFrameFormat m_format;

    class ResourceContext : public QObject
    {
    public:
        ResourceContext() : m_frameFactory(QAndroidVideoFrameFactory::create()) { }
        ~ResourceContext() override { updateLatestImageRef({ }); }
        Q_DISABLE_COPY_MOVE(ResourceContext)

        void updateLatestImageRef(QJniObject newImage)
        {
            auto oldImage = std::exchange(m_latestImage, newImage);
            if (oldImage.isValid())
                oldImage.callMethod<void>("close");
        }

        QVideoFrame latestFrame()
        {
            Q_ASSERT(m_frameFactory);
            return m_latestImage.isValid()
                    ? m_frameFactory->createVideoFrame(std::move(m_latestImage))
                    : QVideoFrame();
        }

    private:
        QJniObject m_latestImage;
        std::shared_ptr<QAndroidVideoFrameFactory> m_frameFactory;
    } m_resourceContext;
};

QAndroidScreenCapture::QAndroidScreenCapture()
    : QPlatformSurfaceCapture(ScreenSource{})
{
}

QAndroidScreenCapture::~QAndroidScreenCapture()
{
}

QVideoFrameFormat QAndroidScreenCapture::frameFormat() const
{
    return m_grabber ? m_grabber->format() : QVideoFrameFormat();
}

bool QAndroidScreenCapture::setActiveInternal(bool active)
{
    if (active == static_cast<bool>(m_grabber))
        return true;

    if (m_grabber) {
        m_grabber.reset();
    } else {
        m_grabber = std::make_unique<Grabber>(*this);
        m_grabber->start();
    }

    return bool(m_grabber) == active;
}

void QAndroidScreenCapture::onNewFrameReceived(QtJniTypes::Image image)
{
    if (m_grabber)
        m_grabber->onNewFrameReceived(image);
    else if (image.isValid())
        image.callMethod<void>("close");
}

static void onScreenFrameAvailable(JNIEnv *env, jobject obj, QtJniTypes::Image image, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    auto *cppObj = reinterpret_cast<QAndroidScreenCapture*>(id);
    cppObj->onNewFrameReceived(image);
}
Q_DECLARE_JNI_NATIVE_METHOD(onScreenFrameAvailable)

static void onErrorUpdate(JNIEnv *env, jobject obj, QString errorString, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    auto cppObj = reinterpret_cast<QAndroidScreenCapture*>(id);
    QMetaObject::invokeMethod(cppObj,
                              &QPlatformSurfaceCapture::updateError,
                              Qt::QueuedConnection,
                              QPlatformSurfaceCapture::Error::InternalError,
                              errorString);
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

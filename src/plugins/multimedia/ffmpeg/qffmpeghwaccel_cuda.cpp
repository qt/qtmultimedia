// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeghwaccel_cuda_p.h"

#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>

#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>

#include <QtGui/rhi/qrhi.h>
#include <QtGui/rhi/qrhi_platform.h>

#include <QtCore/qlibrary.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/quuid.h>
#include <QtCore/private/quniquehandle_types_p.h>

#include <array>
#include <optional>

#if QT_CONFIG(opengl)
#  include <QtGui/qopenglcontext.h>
#  include <QtGui/qopengl.h>
#  include <QtGui/qopenglfunctions.h>
#  ifndef GL_RG16F
#    define GL_RG16F 0x822F
#  endif
#  ifndef GL_RG16
#    define GL_RG16 0x822C
#  endif
#  ifndef GL_R16
#    define GL_R16 0x822A
#  endif
#endif

#if QT_CONFIG(vulkan)
#  include <QtMultimedia/private/qmultimedia_vulkan_support_p.h>
#  include <QtGui/qvulkanfunctions.h>
#  include <QtGui/qvulkaninstance.h>
#  include <vulkan/vulkan.h>
#endif

#if QT_CONFIG(opengl) || QT_CONFIG(vulkan)
#  define QT_FFMPEG_HAS_CUDA_TEXTURE_INTEROP 1
#endif

#if QT_CONFIG(opengl) && (!QT_CONFIG(opengles2) || QT_CONFIG(opengles3))
#  define QT_FFMPEG_HAS_CUDA_GL_TEXTURE_INTEROP 1
#endif

extern "C" {

// dynlink_cuda.h declares the CUDA driver API without requiring a CUDA Toolkit install;
// including it first also makes libavutil/hwcontext_cuda.h skip its own "<cuda.h>" fallback.
// The header is vendored in src/3rdparty/nv-codec-headers, so it is always available.
#include <ffnvcodec/dynlink_cuda.h>

#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_cuda.h>
}

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(qLHWAccelCUDA, "qt.multimedia.ffmpeg.hwaccelcuda")

namespace QFFmpeg {

template <typename T>
using PerPlaneArray = std::array<T, QVideoTextureHelper::TextureDescription::maxPlanes>;

#if QT_CONFIG(opengl) || QT_CONFIG(vulkan)

namespace {

class CudaDriverApi
{
public:
    static const CudaDriverApi *get()
    {
        static std::optional<CudaDriverApi> singleton = []() -> std::optional<CudaDriverApi> {
            CudaDriverApi api;
            if (!api.isValid())
                return std::nullopt;
            return std::optional<CudaDriverApi>{ std::move(api) };
        }();
        return singleton ? &*singleton : nullptr;
    }

    bool pushCurrent(CUcontext ctx) const { return cuCtxPushCurrent_v2(ctx) == CUDA_SUCCESS; }
    bool popCurrent() const
    {
        CUcontext popped = nullptr;
        return cuCtxPopCurrent_v2(&popped) == CUDA_SUCCESS;
    }

    tcuGraphicsGLRegisterImage *cuGraphicsGLRegisterImage = nullptr;
    tcuGraphicsUnregisterResource *cuGraphicsUnregisterResource = nullptr;
    tcuGraphicsMapResources *cuGraphicsMapResources = nullptr;
    tcuGraphicsUnmapResources *cuGraphicsUnmapResources = nullptr;
    tcuGraphicsSubResourceGetMappedArray *cuGraphicsSubResourceGetMappedArray = nullptr;
    tcuMemcpy2D_v2 *cuMemcpy2D_v2 = nullptr;
    tcuCtxPushCurrent_v2 *cuCtxPushCurrent_v2 = nullptr;
    tcuCtxPopCurrent_v2 *cuCtxPopCurrent_v2 = nullptr;
    tcuCtxGetDevice *cuCtxGetDevice = nullptr;
    tcuStreamSynchronize *cuStreamSynchronize = nullptr;
    tcuDeviceGetUuid *cuDeviceGetUuid = nullptr;
    tcuImportExternalMemory *cuImportExternalMemory = nullptr;
    tcuDestroyExternalMemory *cuDestroyExternalMemory = nullptr;
    tcuExternalMemoryGetMappedMipmappedArray *cuExternalMemoryGetMappedMipmappedArray = nullptr;
    tcuMipmappedArrayGetLevel *cuMipmappedArrayGetLevel = nullptr;
    tcuGetErrorString *cuGetErrorString = nullptr;

private:
    CudaDriverApi()
        : m_library{
              std::make_unique<QLibrary>(u"cuda"_s),
          }
    {
        if (!m_library->load()) {
            qCDebug(qLHWAccelCUDA)
                    << "libcuda.so.1 could not be loaded:" << m_library->errorString();
            m_library = nullptr;
            return;
        }

#  define RESOLVE(name)                                                         \
      name = reinterpret_cast<decltype(name)>(m_library->resolve(#name));       \
      if (!name) {                                                              \
          qCDebug(qLHWAccelCUDA) << "libcuda.so.1 is missing symbol:" << #name; \
          m_library = nullptr;                                                  \
          return;                                                               \
      }

        RESOLVE(cuGraphicsGLRegisterImage)
        RESOLVE(cuGraphicsUnregisterResource)
        RESOLVE(cuGraphicsMapResources)
        RESOLVE(cuGraphicsUnmapResources)
        RESOLVE(cuGraphicsSubResourceGetMappedArray)
        RESOLVE(cuMemcpy2D_v2)
        RESOLVE(cuCtxPushCurrent_v2)
        RESOLVE(cuCtxPopCurrent_v2)
        RESOLVE(cuCtxGetDevice)
        RESOLVE(cuStreamSynchronize)
        RESOLVE(cuDeviceGetUuid)
        RESOLVE(cuImportExternalMemory)
        RESOLVE(cuDestroyExternalMemory)
        RESOLVE(cuExternalMemoryGetMappedMipmappedArray)
        RESOLVE(cuMipmappedArrayGetLevel)
        RESOLVE(cuGetErrorString)
#  undef RESOLVE
    }

    bool isValid() const { return bool(m_library); }
    std::unique_ptr<QLibrary> m_library;
};

class CudaContextGuard
{
public:
    static std::optional<CudaContextGuard> create(const CudaDriverApi &api, CUcontext ctx)
    {
        if (!api.pushCurrent(ctx))
            return std::nullopt;
        return CudaContextGuard(api);
    }

    CudaContextGuard(CudaContextGuard &&other) noexcept : m_api(other.m_api)
    {
        other.m_api = nullptr;
    }

    CudaContextGuard &operator=(CudaContextGuard &&) = delete;

    ~CudaContextGuard()
    {
        if (m_api)
            m_api->popCurrent();
    }

    Q_DISABLE_COPY(CudaContextGuard)

private:
    explicit CudaContextGuard(const CudaDriverApi &api) : m_api(&api) { }

    const CudaDriverApi *m_api;
};

class CudaResult
{
public:
    CudaResult(CUresult result, const CudaDriverApi &api) : m_result(result), m_api(api) { }

private:
    friend QDebug operator<<(QDebug debug, const CudaResult &r)
    {
        const QDebugStateSaver saver(debug);
        const char *errorString = nullptr;
        r.m_api.cuGetErrorString(r.m_result, &errorString);
        debug.nospace().noquote() << "CUresult(0x" << Qt::hex << qToUnderlying(r.m_result)
                                  << Qt::dec << ", "
                                  << (errorString ? errorString : "unknown error") << ')';
        return debug;
    }

    CUresult m_result;
    const CudaDriverApi &m_api;
};

// The layout of one plane of a CUDA-decoded pixel format, backend-agnostic: just enough to
// compute plane sizes and the byte width of the CUDA memcpy.
struct CudaPlaneGeometry
{
    int sampleSize = 1; // bytes per sample: 1 for 8-bit formats, 2 for 10/16-bit-in-16-bit
    int numChannels = 1; // 1 (luma) or 2 (interleaved chroma)
    // The plane's size is the frame size divided by these factors, e.g. 2/2 for a 4:2:0
    // chroma plane. Default to 1, not 0, since these are divisors.
    int widthDivisor = 1;
    int heightDivisor = 1;

    int bytesPerTexel() const { return sampleSize * numChannels; }
};

struct CudaFrameGeometry
{
    int planeCount = 0;
    PerPlaneArray<CudaPlaneGeometry> planes{};
};

// NVDEC only ever decodes into semi-planar (2-plane, chroma-subsampled) formats - never
// fully-planar ones - so this table only needs to cover that family.
std::optional<CudaFrameGeometry> cudaFrameGeometry(AVPixelFormat swFormat)
{
    switch (swFormat) {
    case AV_PIX_FMT_NV12:
    case AV_PIX_FMT_NV21:
        return CudaFrameGeometry{
            2,
            { {
                    CudaPlaneGeometry{ 1, 1, 1, 1 },
                    CudaPlaneGeometry{ 1, 2, 2, 2 },
            } },
        };
    case AV_PIX_FMT_P010:
    case AV_PIX_FMT_P016:
        return CudaFrameGeometry{
            2,
            { {
                    CudaPlaneGeometry{ 2, 1, 1, 1 },
                    CudaPlaneGeometry{ 2, 2, 2, 2 },
            } },
        };
    default:
        return std::nullopt;
    }
}

// Exposes each plane's texture as a native handle: a GLuint on OpenGL, a VkImage on Vulkan.
class CudaTextureHandles final : public QVideoFrameTexturesHandles
{
public:
    CudaTextureHandles(int nPlanes, PerPlaneArray<quint64> handles,
                       std::shared_ptr<void> parentKeepAlive)
        : m_nPlanes(nPlanes), m_handles(handles), m_parentKeepAlive(std::move(parentKeepAlive))
    {
    }

    quint64 textureHandle(QRhi &, int plane) override
    {
        return plane < m_nPlanes ? m_handles[plane] : 0;
    }

private:
    const int m_nPlanes;
    const PerPlaneArray<quint64> m_handles;
    const std::shared_ptr<void>
            m_parentKeepAlive; // keeps the converter (and its GL/Vulkan resources) alive
};

// ---------------------------------------------------------------------------------------------
// OpenGL backend
// ---------------------------------------------------------------------------------------------

#  if QT_FFMPEG_HAS_CUDA_GL_TEXTURE_INTEROP

struct GlPlaneFormat
{
    GLenum internalFormat;
    GLenum format;
    GLenum type;
};

GlPlaneFormat glPlaneFormat(const CudaPlaneGeometry &geometry)
{
    const GLenum type = geometry.sampleSize == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
    if (geometry.numChannels == 1)
        return GlPlaneFormat{
            GLenum(geometry.sampleSize == 1 ? GL_R8 : GL_R16),
            GLenum(GL_RED),
            type,
        };
    return GlPlaneFormat{
        GLenum(geometry.sampleSize == 1 ? GL_RG8 : GL_RG16),
        GLenum(GL_RG),
        type,
    };
}

struct GlPlaneResource
{
    ~GlPlaneResource() { Q_ASSERT(texture == 0 && !cudaResource); }

    void release(const CudaDriverApi *cudaApi, QOpenGLFunctions &functions)
    {
        if (cudaResource && cudaApi)
            cudaApi->cuGraphicsUnregisterResource(cudaResource);
        if (texture)
            functions.glDeleteTextures(1, &texture);
        cudaResource = nullptr;
        texture = 0;
    }

    GLuint texture = 0;
    CUgraphicsResource cudaResource = nullptr;
};

class CUDAGLTextureConverter final : public TextureConverterBackend
{
public:
    static std::shared_ptr<TextureConverterBackend> create(QRhi *rhi)
    {
        if (!rhi)
            return nullptr;

        auto *api = CudaDriverApi::get();
        if (!api) {
            qCDebug(qLHWAccelCUDA) << "CUDA driver API is not available, disabling";
            return nullptr;
        }

        return std::make_shared<CUDAGLTextureConverter>(rhi, api);
    }

    explicit CUDAGLTextureConverter(QRhi *rhi, const CudaDriverApi *api)
        : TextureConverterBackend(rhi), m_api{ api }
    {
    }

    ~CUDAGLTextureConverter() override { releasePlaneResources(); }

    QVideoFrameTexturesHandlesUPtr
    createTextureHandles(AVFrame *frame, QVideoFrameTexturesHandlesUPtr /*oldHandles*/) override
    {
        if (frame->format != AV_PIX_FMT_CUDA || !frame->hw_frames_ctx)
            return nullptr;

        auto *framesCtx = reinterpret_cast<AVHWFramesContext *>(frame->hw_frames_ctx->data);
        const std::optional<CudaFrameGeometry> geometry = cudaFrameGeometry(framesCtx->sw_format);
        if (!geometry) {
            qWarning() << "CUDATextureConverter does not support sw_format" << framesCtx->sw_format;
            return nullptr;
        }

        auto *deviceCtx = avFrameDeviceContext(frame);
        if (!deviceCtx)
            return nullptr;
        auto *cudaCtx = reinterpret_cast<AVCUDADeviceContext *>(deviceCtx->hwctx);

        std::optional<CudaContextGuard> guard = CudaContextGuard::create(*m_api, cudaCtx->cuda_ctx);
        if (!guard) {
            qWarning() << "Failed to push the CUDA context current; disabling";
            rhi = nullptr;
            return nullptr;
        }
        m_cudaContext = cudaCtx->cuda_ctx;

        if (!ensurePlaneResources(frame))
            return nullptr;

        PerPlaneArray<CUgraphicsResource> resources{};
        for (int i = 0; i < geometry->planeCount; ++i)
            resources[i] = m_planes[i]->cudaResource;
        const CUresult mapResult =
                m_api->cuGraphicsMapResources(geometry->planeCount, resources.data(), nullptr);
        if (mapResult != CUDA_SUCCESS) {
            qWarning() << "cuGraphicsMapResources failed:" << CudaResult(mapResult, *m_api);
            return nullptr;
        }

        auto unmapGuard = qScopeGuard([&] {
            m_api->cuGraphicsUnmapResources(geometry->planeCount, resources.data(), nullptr);
        });

        PerPlaneArray<quint64> handles{};
        for (int i = 0; i < geometry->planeCount; ++i) {
            const CudaPlaneGeometry &planeGeometry = geometry->planes[i];
            const QSize planeSize(frame->width / planeGeometry.widthDivisor,
                                  frame->height / planeGeometry.heightDivisor);

            CUarray array = nullptr;
            const CUresult subResourceResult =
                    m_api->cuGraphicsSubResourceGetMappedArray(&array, resources[i], 0, 0);
            if (subResourceResult != CUDA_SUCCESS) {
                qWarning() << "cuGraphicsSubResourceGetMappedArray failed for plane" << i << ":"
                           << CudaResult(subResourceResult, *m_api);
                return nullptr;
            }

            const CUDA_MEMCPY2D copy{
                .srcXInBytes = 0,
                .srcY = 0,
                .srcMemoryType = CU_MEMORYTYPE_DEVICE,
                .srcHost = nullptr,
                .srcDevice = reinterpret_cast<CUdeviceptr>(frame->data[i]),
                .srcArray = nullptr,
                .srcPitch = size_t(frame->linesize[i]),
                .dstXInBytes = 0,
                .dstY = 0,
                .dstMemoryType = CU_MEMORYTYPE_ARRAY,
                .dstHost = nullptr,
                .dstDevice = 0,
                .dstArray = array,
                .dstPitch = 0,
                .WidthInBytes = size_t(planeSize.width()) * size_t(planeGeometry.bytesPerTexel()),
                .Height = size_t(planeSize.height()),
            };

            const CUresult copyResult = m_api->cuMemcpy2D_v2(&copy);
            if (copyResult != CUDA_SUCCESS) {
                qWarning() << "cuMemcpy2D_v2 failed for plane" << i << ":"
                           << CudaResult(copyResult, *m_api);
                return nullptr;
            }

            handles[i] = quint64(m_planes[i]->texture);
        }

        return std::make_unique<CudaTextureHandles>(geometry->planeCount, handles,
                                                    shared_from_this());
    }

private:
    bool ensurePlaneResources(AVFrame *frame)
    {
        auto *framesCtx = reinterpret_cast<AVHWFramesContext *>(frame->hw_frames_ctx->data);
        const std::optional<CudaFrameGeometry> maybeGeometry =
                cudaFrameGeometry(framesCtx->sw_format);
        Q_ASSERT(maybeGeometry); // already validated by createTextureHandles
        const CudaFrameGeometry &geometry = *maybeGeometry;

        const QSize frameSize(frame->width, frame->height);
        const bool canReuse = frameSize == m_planeResourcesSize
                && framesCtx->sw_format == m_planeResourcesFormat
                && ranges::all_of(QSpan(m_planes.data(), geometry.planeCount),
                                  [](const auto &plane) {
            return bool(plane);
        });
        if (canReuse)
            return true;

        releasePlaneResources();
        m_planeResourcesSize = frameSize;
        m_planeResourcesFormat = framesCtx->sw_format;

        auto *glContext =
                static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles())->context;
        QOpenGLFunctions functions(glContext);

        for (int i = 0; i < geometry.planeCount; ++i) {
            const CudaPlaneGeometry &planeGeometry = geometry.planes[i];
            const QSize planeSize(m_planeResourcesSize.width() / planeGeometry.widthDivisor,
                                  m_planeResourcesSize.height() / planeGeometry.heightDivisor);
            const GlPlaneFormat format = glPlaneFormat(planeGeometry);

            auto plane = std::make_unique<GlPlaneResource>();
            functions.glGenTextures(1, &plane->texture);
            functions.glBindTexture(GL_TEXTURE_2D, plane->texture);
            functions.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            functions.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            functions.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            functions.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            functions.glTexImage2D(GL_TEXTURE_2D, 0, format.internalFormat, planeSize.width(),
                                   planeSize.height(), 0, format.format, format.type, nullptr);
            functions.glBindTexture(GL_TEXTURE_2D, 0);

            const CUresult result = m_api->cuGraphicsGLRegisterImage(
                    &plane->cudaResource, plane->texture, GL_TEXTURE_2D,
                    CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD);
            if (result != CUDA_SUCCESS) {
                qWarning() << "cuGraphicsGLRegisterImage failed for plane" << i << ":"
                           << CudaResult(result, *m_api);
                plane->release(m_api, functions);
                return false;
            }

            m_planes[i] = std::move(plane);
        }

        return true;
    }

    void releasePlaneResources()
    {
        if (!rhi)
            return;

        std::optional<CudaContextGuard> guard = m_cudaContext && m_api
                ? CudaContextGuard::create(*m_api, m_cudaContext)
                : std::nullopt;

        rhi->makeThreadLocalNativeContextCurrent();
        auto *glContext =
                static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles())->context;
        QOpenGLFunctions functions(glContext);

        for (auto &plane : m_planes) {
            if (!plane)
                continue;
            plane->release(guard ? m_api : nullptr, functions);
            plane.reset();
        }
    }

    const CudaDriverApi *const m_api;
    PerPlaneArray<std::unique_ptr<GlPlaneResource>> m_planes;
    QSize m_planeResourcesSize;
    AVPixelFormat m_planeResourcesFormat = AV_PIX_FMT_NONE;
    CUcontext m_cudaContext = nullptr;
};

#  endif // QT_FFMPEG_HAS_CUDA_GL_TEXTURE_INTEROP

// ---------------------------------------------------------------------------------------------
// Vulkan backend
// ---------------------------------------------------------------------------------------------

#  if QT_CONFIG(vulkan)

// Pass a device to resolve a device-level function (e.g. of VK_KHR_external_memory_fd), which
// yields nullptr if the extension is not enabled on that device. Without a device, an
// instance-level function is resolved.
template <typename Func>
Func resolveVulkanFunctionOrWarn(QVulkanInstance &inst, const char *name,
                                 VkDevice dev = VK_NULL_HANDLE)
{
    using namespace QtMultimediaPrivate;
    Func func = dev ? resolveVulkanDeviceFunction<Func>(inst, dev, name)
                    : resolveVulkanFunction<Func>(inst, name);
    if (!func)
        qCDebug(qLHWAccelCUDA) << "Failed to resolve Vulkan function" << name;
    return func;
}

// vkGetPhysicalDeviceProperties2 is core since Vulkan 1.1, with vkGetPhysicalDeviceProperties2KHR
// as the (functionally identical) extension entry point on older API versions/instances.
PFN_vkGetPhysicalDeviceProperties2 resolveVkGetPhysicalDeviceProperties2(QVulkanInstance &inst)
{
    using namespace QtMultimediaPrivate;
    PFN_vkGetPhysicalDeviceProperties2 func =
            resolveVulkanFunction<PFN_vkGetPhysicalDeviceProperties2>(
                    inst, "vkGetPhysicalDeviceProperties2");
    if (!func) {
        func = resolveVulkanFunctionOrWarn<PFN_vkGetPhysicalDeviceProperties2>(
                inst, "vkGetPhysicalDeviceProperties2KHR");
    }
    return func;
}

QUuid uuidFromCUuuid(const CUuuid &cuUuid)
{
    return QUuid::fromRfc4122(QByteArrayView(reinterpret_cast<const char *>(cuUuid.bytes), 16));
}

// Resolves the Vulkan state/functions needed to export a plane's VkImage memory as a POSIX
// fd (VK_KHR_external_memory_fd) for import into CUDA. Bound to one QRhi::Vulkan instance.
class CudaVulkanInterop
{
public:
    explicit CudaVulkanInterop(const QRhiVulkanNativeHandles &handles)
        : inst(handles.inst),
          physDev(handles.physDev),
          dev(handles.dev),
          deviceFuncs(handles.inst->deviceFunctions(handles.dev)),
          instFuncs(handles.inst->functions()),
          vkGetMemoryFdKHR(resolveVulkanFunctionOrWarn<PFN_vkGetMemoryFdKHR>(
                  *handles.inst, "vkGetMemoryFdKHR", handles.dev)),
          vkGetPhysicalDeviceProperties2(resolveVkGetPhysicalDeviceProperties2(*handles.inst))
    {
    }

    static std::unique_ptr<CudaVulkanInterop> create(QRhi *rhi)
    {
        Q_ASSERT(rhi->backend() == QRhi::Vulkan);
        const auto *handles = static_cast<const QRhiVulkanNativeHandles *>(rhi->nativeHandles());
        if (!handles || !handles->inst || !handles->dev || !handles->physDev)
            return nullptr;

        auto interop = std::make_unique<CudaVulkanInterop>(*handles);
        if (!interop->deviceFuncs || !interop->instFuncs)
            return nullptr;

        if (!interop->vkGetMemoryFdKHR || !interop->vkGetPhysicalDeviceProperties2) {
            qCDebug(qLHWAccelCUDA)
                    << "VK_KHR_external_memory_fd or vkGetPhysicalDeviceProperties2 is either "
                       "unavailable or not enabled on this QRhi's VkDevice/VkInstance, disabling. "
                       "To use the CUDA texture interop with Vulkan, enable the device extensions "
                       "VK_KHR_external_memory and VK_KHR_external_memory_fd when creating the "
                       "QRhi, e.g. with QQuickGraphicsConfiguration::setDeviceExtensions() or "
                       "QRhiVulkanInitParams::deviceExtensions";
            return nullptr;
        }

        return interop;
    }

    enum class MatchResult : uint8_t {
        SameGpu,
        DifferentGpu,
        Unknown, // Vulkan device UUID could not be queried
    };
    MatchResult matchesCudaDevice(const CUuuid &cudaUuid) const
    {
        using namespace QtMultimediaPrivate;

        VkPhysicalDeviceIDProperties idProps{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
            .pNext = nullptr,
            .deviceUUID = {},
            .driverUUID = {},
            .deviceLUID = {},
            .deviceNodeMask = 0,
            .deviceLUIDValid = VK_FALSE,
        };
        VkPhysicalDeviceProperties2 props2{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &idProps,
            .properties = {},
        };
        vkGetPhysicalDeviceProperties2(physDev, &props2);
        const QUuid vkDeviceUuid = uuidFromVkUuid(idProps.deviceUUID);
        if (vkDeviceUuid.isNull())
            return MatchResult::Unknown;
        const QUuid cuDeviceUuid = uuidFromCUuuid(cudaUuid);
        return (vkDeviceUuid == cuDeviceUuid) ? MatchResult::SameGpu : MatchResult::DifferentGpu;
    }

    std::optional<uint32_t> findDeviceLocalMemoryTypeIndex(uint32_t memoryTypeBits) const
    {
        VkPhysicalDeviceMemoryProperties memProps{};
        instFuncs->vkGetPhysicalDeviceMemoryProperties(physDev, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            const bool allowed = memoryTypeBits & (1u << i);
            const bool deviceLocal =
                    memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            if (allowed && deviceLocal)
                return i;
        }
        return std::nullopt;
    }

    QVulkanInstance *const inst;
    const VkPhysicalDevice physDev;
    const VkDevice dev;
    QVulkanDeviceFunctions *const deviceFuncs;
    QVulkanFunctions *const instFuncs;
    const PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;
    const PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2;
};

struct VulkanPlaneFormat
{
    VkFormat vkFormat;
    CUarray_format cuArrayFormat;
};

VulkanPlaneFormat vulkanPlaneFormat(const CudaPlaneGeometry &geometry)
{
    const CUarray_format cuFormat =
            geometry.sampleSize == 1 ? CU_AD_FORMAT_UNSIGNED_INT8 : CU_AD_FORMAT_UNSIGNED_INT16;
    if (geometry.numChannels == 1)
        return VulkanPlaneFormat{
            geometry.sampleSize == 1 ? VK_FORMAT_R8_UNORM : VK_FORMAT_R16_UNORM,
            cuFormat,
        };
    return VulkanPlaneFormat{
        geometry.sampleSize == 1 ? VK_FORMAT_R8G8_UNORM : VK_FORMAT_R16G16_UNORM,
        cuFormat,
    };
}

// Vulkan interop (VK_KHR_external_memory_fd + cuImportExternalMemory).
struct VulkanPlaneResource
{
    ~VulkanPlaneResource()
    {
        Q_ASSERT(image == VK_NULL_HANDLE && memory == VK_NULL_HANDLE && !externalMemory);
    }

    void release(const CudaDriverApi *cudaApi, CudaVulkanInterop &vulkan)
    {
        if (externalMemory && cudaApi)
            cudaApi->cuDestroyExternalMemory(externalMemory);
        if (memory != VK_NULL_HANDLE)
            vulkan.deviceFuncs->vkFreeMemory(vulkan.dev, memory, nullptr);
        if (image != VK_NULL_HANDLE)
            vulkan.deviceFuncs->vkDestroyImage(vulkan.dev, image, nullptr);
        externalMemory = nullptr;
        mipmappedArray = nullptr;
        array = nullptr;
        memory = VK_NULL_HANDLE;
        image = VK_NULL_HANDLE;
    }

    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    CUexternalMemory externalMemory = nullptr;
    CUmipmappedArray mipmappedArray = nullptr;
    CUarray array = nullptr; // level-0 view of mipmappedArray, cached for the copy
};

class CUDAVulkanTextureConverter final : public TextureConverterBackend
{
    using MatchResult = CudaVulkanInterop::MatchResult;

public:
    static std::shared_ptr<TextureConverterBackend> create(QRhi *rhi)
    {
        if (!rhi)
            return nullptr;

        auto *api = CudaDriverApi::get();
        if (!api) {
            qCDebug(qLHWAccelCUDA) << "CUDA driver API is not available, disabling";
            return nullptr;
        }

        auto vulkanApi = CudaVulkanInterop::create(rhi);
        if (!vulkanApi) {
            qCDebug(qLHWAccelCUDA) << "CUDA-Vulkan interop is not available, disabling";
            return nullptr;
        }

        return std::make_shared<CUDAVulkanTextureConverter>(rhi, api, std::move(vulkanApi));
    }

    explicit CUDAVulkanTextureConverter(QRhi *rhi, const CudaDriverApi *api,
                                        std::unique_ptr<CudaVulkanInterop> vulkanApi)
        : TextureConverterBackend(rhi), m_api{ api }, m_vulkan{ std::move(vulkanApi) }
    {
    }

    ~CUDAVulkanTextureConverter() override { releasePlaneResources(); }

    QVideoFrameTexturesHandlesUPtr
    createTextureHandles(AVFrame *frame, QVideoFrameTexturesHandlesUPtr /*oldHandles*/) override
    {
        if (frame->format != AV_PIX_FMT_CUDA || !frame->hw_frames_ctx)
            return nullptr;

        auto *framesCtx = reinterpret_cast<AVHWFramesContext *>(frame->hw_frames_ctx->data);
        const std::optional<CudaFrameGeometry> geometry = cudaFrameGeometry(framesCtx->sw_format);
        if (!geometry) {
            qWarning() << "CUDATextureConverter does not support sw_format" << framesCtx->sw_format;
            return nullptr;
        }

        auto *deviceCtx = avFrameDeviceContext(frame);
        if (!deviceCtx)
            return nullptr;
        auto *cudaCtx = reinterpret_cast<AVCUDADeviceContext *>(deviceCtx->hwctx);

        std::optional<CudaContextGuard> guard = CudaContextGuard::create(*m_api, cudaCtx->cuda_ctx);
        if (!guard) {
            qWarning() << "Failed to push the CUDA context current; disabling";
            rhi = nullptr;
            return nullptr;
        }
        m_cudaContext = cudaCtx->cuda_ctx;

        if (!ensurePlaneResources(frame))
            return nullptr;

        PerPlaneArray<quint64> handles{};
        for (int i = 0; i < geometry->planeCount; ++i) {
            const CudaPlaneGeometry &planeGeometry = geometry->planes[i];
            const QSize planeSize(frame->width / planeGeometry.widthDivisor,
                                  frame->height / planeGeometry.heightDivisor);

            const CUDA_MEMCPY2D copy{
                .srcXInBytes = 0,
                .srcY = 0,
                .srcMemoryType = CU_MEMORYTYPE_DEVICE,
                .srcHost = nullptr,
                .srcDevice = reinterpret_cast<CUdeviceptr>(frame->data[i]),
                .srcArray = nullptr,
                .srcPitch = size_t(frame->linesize[i]),
                .dstXInBytes = 0,
                .dstY = 0,
                .dstMemoryType = CU_MEMORYTYPE_ARRAY,
                .dstHost = nullptr,
                .dstDevice = 0,
                .dstArray = m_planes[i]->array,
                .dstPitch = 0,
                .WidthInBytes = size_t(planeSize.width()) * size_t(planeGeometry.bytesPerTexel()),
                .Height = size_t(planeSize.height()),
            };

            const CUresult copyResult = m_api->cuMemcpy2D_v2(&copy);
            if (copyResult != CUDA_SUCCESS) {
                qWarning() << "cuMemcpy2D_v2 failed for plane" << i << ":"
                           << CudaResult(copyResult, *m_api);
                return nullptr;
            }

            handles[i] = quint64(m_planes[i]->image);
        }

        // No external-semaphore pipelining (see the class docstring): block until the
        // copies actually land before handing the VkImages to the RHI. cuMemcpy2D_v2
        // runs on the default stream, so synchronizing that stream is sufficient.
        const CUresult syncResult = m_api->cuStreamSynchronize(nullptr);
        if (syncResult != CUDA_SUCCESS) {
            qWarning() << "cuStreamSynchronize failed:" << CudaResult(syncResult, *m_api);
            return nullptr;
        }

        return std::make_unique<CudaTextureHandles>(geometry->planeCount, handles,
                                                    shared_from_this());
    }

private:
    bool ensurePlaneResources(AVFrame *frame)
    {
        auto *framesCtx = reinterpret_cast<AVHWFramesContext *>(frame->hw_frames_ctx->data);
        const std::optional<CudaFrameGeometry> maybeGeometry =
                cudaFrameGeometry(framesCtx->sw_format);
        Q_ASSERT(maybeGeometry); // already validated by createTextureHandles
        const CudaFrameGeometry &geometry = *maybeGeometry;

        const QSize frameSize(frame->width, frame->height);
        const bool canReuse = frameSize == m_planeResourcesSize
                && framesCtx->sw_format == m_planeResourcesFormat
                && ranges::all_of(QSpan(m_planes.data(), geometry.planeCount),
                                  [](const auto &plane) {
            return bool(plane);
        });
        if (canReuse)
            return true;

        releasePlaneResources();
        m_planeResourcesSize = frameSize;
        m_planeResourcesFormat = framesCtx->sw_format;

        if (m_vulkanDeviceMatches == MatchResult::Unknown) {
            CUdevice device = 0;
            CUuuid cudaUuid{};
            const bool ok = m_api->cuCtxGetDevice(&device) == CUDA_SUCCESS
                    && m_api->cuDeviceGetUuid(&cudaUuid, device) == CUDA_SUCCESS;
            m_vulkanDeviceMatches =
                    ok ? m_vulkan->matchesCudaDevice(cudaUuid) : MatchResult::Unknown;
            if (m_vulkanDeviceMatches == MatchResult::Unknown) {
                qWarning() << "CUDATextureConverter: could not determine the QRhi's Vulkan "
                              "physical device UUID (missing "
                              "VK_KHR_get_physical_device_properties2 on the VkInstance?); "
                              "disabling";
            } else if (m_vulkanDeviceMatches == MatchResult::DifferentGpu) {
                qWarning() << "CUDATextureConverter: the CUDA device does not match the QRhi's "
                              "Vulkan physical device (e.g. a multi-GPU system); disabling";
            }
        }
        if (m_vulkanDeviceMatches != MatchResult::SameGpu)
            return false;

        for (int i = 0; i < geometry.planeCount; ++i) {
            const CudaPlaneGeometry &planeGeometry = geometry.planes[i];
            const QSize planeSize(m_planeResourcesSize.width() / planeGeometry.widthDivisor,
                                  m_planeResourcesSize.height() / planeGeometry.heightDivisor);
            const VulkanPlaneFormat format = vulkanPlaneFormat(planeGeometry);

            auto plane = std::make_unique<VulkanPlaneResource>();
            auto releaseGuard = qScopeGuard([&] {
                plane->release(m_api, *m_vulkan);
            });

            const VkExternalMemoryImageCreateInfo extImageInfo{
                .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
            };

            // TRANSFER_SRC is not needed by the RHI (it only samples this texture), but it
            // costs nothing to declare and lets tests validate the copy via a readback.
            const VkImageCreateInfo imageInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = &extImageInfo,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = format.vkFormat,
                .extent =
                        VkExtent3D{
                                .width = uint32_t(planeSize.width()),
                                .height = uint32_t(planeSize.height()),
                                .depth = 1u,
                        },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };

            if (m_vulkan->deviceFuncs->vkCreateImage(m_vulkan->dev, &imageInfo, nullptr,
                                                     &plane->image)
                != VK_SUCCESS) {
                qWarning() << "CUDATextureConverter: vkCreateImage failed for plane" << i;
                return false;
            }

            VkMemoryRequirements memReq{};
            m_vulkan->deviceFuncs->vkGetImageMemoryRequirements(m_vulkan->dev, plane->image,
                                                                &memReq);

            const std::optional<uint32_t> memoryTypeIndex =
                    m_vulkan->findDeviceLocalMemoryTypeIndex(memReq.memoryTypeBits);
            if (!memoryTypeIndex) {
                qWarning() << "CUDATextureConverter: no suitable Vulkan memory type for plane" << i;
                return false;
            }

            const VkExportMemoryAllocateInfo exportInfo{
                .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
            };

            const VkMemoryAllocateInfo allocInfo{
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = &exportInfo,
                .allocationSize = memReq.size,
                .memoryTypeIndex = *memoryTypeIndex,
            };

            if (m_vulkan->deviceFuncs->vkAllocateMemory(m_vulkan->dev, &allocInfo, nullptr,
                                                        &plane->memory)
                != VK_SUCCESS) {
                qWarning() << "CUDATextureConverter: vkAllocateMemory failed for plane" << i;
                return false;
            }

            if (m_vulkan->deviceFuncs->vkBindImageMemory(m_vulkan->dev, plane->image, plane->memory,
                                                         0)
                != VK_SUCCESS) {
                qWarning() << "CUDATextureConverter: vkBindImageMemory failed for plane" << i;
                return false;
            }

            const VkMemoryGetFdInfoKHR getFdInfo{
                .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
                .pNext = nullptr,
                .memory = plane->memory,
                .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
            };
            QUniqueFileDescriptorHandle fd;
            if (m_vulkan->vkGetMemoryFdKHR(m_vulkan->dev, &getFdInfo, &fd) != VK_SUCCESS) {
                qWarning() << "CUDATextureConverter: vkGetMemoryFdKHR failed for plane" << i;
                return false;
            }

            const CUDA_EXTERNAL_MEMORY_HANDLE_DESC extMemDesc{
                .type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD,
                .handle = { .fd = fd.get() },
                .size = memReq.size,
                .flags = 0,
                .reserved = {},
            };
            const CUresult importResult =
                    m_api->cuImportExternalMemory(&plane->externalMemory, &extMemDesc);
            if (importResult != CUDA_SUCCESS) {
                qWarning() << "CUDATextureConverter: cuImportExternalMemory failed for plane" << i
                           << ":" << CudaResult(importResult, *m_api);
                return false;
            }
            std::ignore = fd.release(); // import succeeded, ownership transferred to CUDA

            const CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC arrayDesc{
                .offset = 0,
                .arrayDesc =
                        CUDA_ARRAY3D_DESCRIPTOR{
                                .Width = size_t(planeSize.width()),
                                .Height = size_t(planeSize.height()),
                                .Depth = 0,
                                .Format = format.cuArrayFormat,
                                .NumChannels = unsigned(planeGeometry.numChannels),
                                .Flags = 0,
                        },
                .numLevels = 1,
                .reserved = {},
            };

            const CUresult mappedArrayResult = m_api->cuExternalMemoryGetMappedMipmappedArray(
                    &plane->mipmappedArray, plane->externalMemory, &arrayDesc);
            if (mappedArrayResult != CUDA_SUCCESS) {
                qWarning() << "CUDATextureConverter: cuExternalMemoryGetMappedMipmappedArray "
                              "failed for plane"
                           << i << ":" << CudaResult(mappedArrayResult, *m_api);
                return false;
            }

            const CUresult arrayLevelResult =
                    m_api->cuMipmappedArrayGetLevel(&plane->array, plane->mipmappedArray, 0);
            if (arrayLevelResult != CUDA_SUCCESS) {
                qWarning() << "CUDATextureConverter: cuMipmappedArrayGetLevel failed for plane" << i
                           << ":" << CudaResult(arrayLevelResult, *m_api);
                return false;
            }

            releaseGuard.dismiss();
            m_planes[i] = std::move(plane);
        }

        return true;
    }

    void releasePlaneResources()
    {
        if (!rhi)
            return;

        std::optional<CudaContextGuard> guard = m_cudaContext && m_api
                ? CudaContextGuard::create(*m_api, m_cudaContext)
                : std::nullopt;

        for (auto &plane : m_planes) {
            if (!plane)
                continue;
            plane->release(guard ? m_api : nullptr, *m_vulkan);
            plane.reset();
        }
    }

    const CudaDriverApi *const m_api;
    const std::unique_ptr<CudaVulkanInterop> m_vulkan;
    MatchResult m_vulkanDeviceMatches = MatchResult::Unknown;
    PerPlaneArray<std::unique_ptr<VulkanPlaneResource>> m_planes;
    QSize m_planeResourcesSize;
    AVPixelFormat m_planeResourcesFormat = AV_PIX_FMT_NONE;
    CUcontext m_cudaContext = nullptr;
};

#  endif // QT_CONFIG(vulkan)

} // namespace

#endif // QT_FFMPEG_HAS_CUDA_TEXTURE_INTEROP

// ---------------------------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------------------------

std::shared_ptr<TextureConverterBackend> CUDATextureConverterFactory::create(QRhi *rhi)
{
    if (rhi) {
        switch (rhi->backend()) {
#if QT_FFMPEG_HAS_CUDA_GL_TEXTURE_INTEROP
        case QRhi::OpenGLES2:
            return CUDAGLTextureConverter::create(rhi);
#endif
#if QT_CONFIG(vulkan)
        case QRhi::Vulkan:
            return CUDAVulkanTextureConverter::create(rhi);
#endif
        default:
            break;
        }
    }

    return nullptr;
}

} // namespace QFFmpeg

QT_END_NAMESPACE

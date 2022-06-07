// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "qsgvivantevideomaterial.h"
#include "qsgvivantevideomaterialshader.h"
#include "qsgvivantevideonode.h"
#include "private/qsgvideotexture_p.h"

#include <QOpenGLContext>
#include <QThread>

#include <unistd.h>

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include "private/qgstvideobuffer_p.h"
#if GST_CHECK_VERSION(1,14,0)
#include <gst/allocators/gstphysmemory.h>
#endif

//#define QT_VIVANTE_VIDEO_DEBUG

QSGVivanteVideoMaterial::QSGVivanteVideoMaterial() :
    mOpacity(1.0),
    mWidth(0),
    mHeight(0),
    mFormat(QVideoFrameFormat::Format_Invalid),
    mCurrentTexture(0),
    mMappable(true),
    mTexDirectTexture(0)
{
#ifdef QT_VIVANTE_VIDEO_DEBUG
    qDebug() << Q_FUNC_INFO;
#endif

    setFlag(Blending, false);

    mShader = new QSGVivanteVideoMaterialShader;
}

QSGVivanteVideoMaterial::~QSGVivanteVideoMaterial()
{
    clearTextures();
}

QSGMaterialType *QSGVivanteVideoMaterial::type() const {
    static QSGMaterialType theType;
    return &theType;
}

QSGMaterialShader *QSGVivanteVideoMaterial::createShader(QSGRendererInterface::RenderMode) const {
    return mShader;
}

int QSGVivanteVideoMaterial::compare(const QSGMaterial *other) const {
    if (this->type() == other->type()) {
        const QSGVivanteVideoMaterial *m = static_cast<const QSGVivanteVideoMaterial *>(other);
        if (this->mBitsToTextureMap == m->mBitsToTextureMap)
            return 0;
        else
            return 1;
    }
    return 1;
}

void QSGVivanteVideoMaterial::updateBlending() {
    setFlag(Blending, qFuzzyCompare(mOpacity, qreal(1.0)) ? false : true);
}

void QSGVivanteVideoMaterial::setCurrentFrame(const QVideoFrame &frame, QSGVideoNode::FrameFlags flags)
{
    QMutexLocker lock(&mFrameMutex);
    mCurrentFrame = frame;
    mMappable = mMapError == GL_NO_ERROR;

#ifdef QT_VIVANTE_VIDEO_DEBUG
    qDebug() << Q_FUNC_INFO << " new frame: " << frame;
#endif
}

void QSGVivanteVideoMaterial::bind()
{
    QOpenGLContext *glcontext = QOpenGLContext::currentContext();
    if (glcontext == 0) {
        qWarning() << Q_FUNC_INFO << "no QOpenGLContext::currentContext() => return";
        return;
    }

    QMutexLocker lock(&mFrameMutex);
    if (mCurrentFrame.isValid())
        mCurrentTexture = vivanteMapping(mCurrentFrame);
    else
        glBindTexture(GL_TEXTURE_2D, mCurrentTexture);
}

void QSGVivanteVideoMaterial::clearTextures()
{
    for (auto it = mBitsToTextureMap.cbegin(), end = mBitsToTextureMap.cend(); it != end; ++it) {
        GLuint id = it.value();
#ifdef QT_VIVANTE_VIDEO_DEBUG
        qDebug() << "delete texture: " << id;
#endif
        glDeleteTextures(1, &id);
    }
    mBitsToTextureMap.clear();

    if (mTexDirectTexture) {
        glDeleteTextures(1, &mTexDirectTexture);
        mTexDirectTexture = 0;
    }
}

GLuint QSGVivanteVideoMaterial::vivanteMapping(QVideoFrame vF)
{
    QOpenGLContext *glcontext = QOpenGLContext::currentContext();
    if (glcontext == 0) {
        qWarning() << Q_FUNC_INFO << "no QOpenGLContext::currentContext() => return 0";
        return 0;
    }

    static PFNGLTEXDIRECTVIVPROC glTexDirectVIV_LOCAL = 0;
    static PFNGLTEXDIRECTVIVMAPPROC glTexDirectVIVMap_LOCAL = 0;
    static PFNGLTEXDIRECTINVALIDATEVIVPROC glTexDirectInvalidateVIV_LOCAL = 0;

    if (glTexDirectVIV_LOCAL == 0 || glTexDirectVIVMap_LOCAL == 0 || glTexDirectInvalidateVIV_LOCAL == 0) {
        glTexDirectVIV_LOCAL = reinterpret_cast<PFNGLTEXDIRECTVIVPROC>(glcontext->getProcAddress("glTexDirectVIV"));
        glTexDirectVIVMap_LOCAL = reinterpret_cast<PFNGLTEXDIRECTVIVMAPPROC>(glcontext->getProcAddress("glTexDirectVIVMap"));
        glTexDirectInvalidateVIV_LOCAL = reinterpret_cast<PFNGLTEXDIRECTINVALIDATEVIVPROC>(glcontext->getProcAddress("glTexDirectInvalidateVIV"));
    }
    if (glTexDirectVIV_LOCAL == 0 || glTexDirectVIVMap_LOCAL == 0 || glTexDirectInvalidateVIV_LOCAL == 0) {
        qWarning() << Q_FUNC_INFO << "couldn't find \"glTexDirectVIVMap\" and/or \"glTexDirectInvalidateVIV\" => do nothing and return";
        return 0;
    }

    if (mWidth != vF.width() || mHeight != vF.height() || mFormat != vF.pixelFormat()) {
        mWidth = vF.width();
        mHeight = vF.height();
        mFormat = vF.pixelFormat();
        mMapError = GL_NO_ERROR;
        clearTextures();
    }

    if (vF.map(QVideoFrame::ReadOnly)) {

        if (mMappable) {
            if (!mBitsToTextureMap.contains(vF.bits())) {
                // Haven't yet seen this logical address: map to texture.
                GLuint tmpTexId;
                glGenTextures(1, &tmpTexId);
                mBitsToTextureMap.insert(vF.bits(), tmpTexId);

                // Determine the full width & height. Full means: actual width/height plus extra padding pixels.
                // The full width can be deduced from the bytesPerLine value. The full height is calculated
                // by calculating the distance between the start of the first and second planes, and dividing
                // it by the stride (= the bytesPerLine). If there is only one plane, we don't worry about
                // extra padding rows, since there are no adjacent extra planes.
                // XXX: This assumes the distance between bits(1) and bits(0) is exactly the size of the first
                // plane (the Y plane in the case of YUV data). A better way would be to have a dedicated
                // planeSize() or planeOffset() getter.
                // Also, this assumes that planes are tightly packed, that is, there is no space between them.
                // It is okay to assume this here though, because the Vivante direct textures also assume that.
                // In other words, if the planes aren't tightly packed, then the direct textures won't be able
                // to render the frame correctly anyway.
                int fullWidth = vF.bytesPerLine() / QSGVivanteVideoNode::getBytesForPixelFormat(vF.pixelFormat());
                int fullHeight = (vF.planeCount() > 1) ? ((vF.bits(1) - vF.bits(0)) / vF.bytesPerLine()) : vF.height();

                // The uscale is the ratio of actual width to the full width (same for vscale and height).
                // Since the vivante direct textures do not offer a way to explicitly specify the amount of padding
                // columns and rows, we use a trick. We show the full frame - including the padding pixels - in the
                // texture, but render only a subset of that texture. This subset goes from (0,0) to (uScale, vScale).
                // In the shader, the texture coordinates (which go from (0.0, 0.0) to (1.0, 1.0)) are multiplied by
                // the u/v scale values. Since 1.0 * x = x, this effectively limits the texture coordinates from
                // (0.0, 0.0) - (1.0, 1.0) to (0.0, 0.0) - (uScale, vScale).
                float uScale = float(vF.width()) / float(fullWidth);
                float vScale = float(vF.height()) / float(fullHeight);
                mShader->setUVScale(uScale, vScale);

                const uchar *constBits = vF.bits();
                void *bits = (void*)constBits;

#ifdef QT_VIVANTE_VIDEO_DEBUG
                qDebug() << Q_FUNC_INFO
                         << "new texture, texId: " << tmpTexId
                         << "; constBits: " << constBits
                         << "; actual/full width: " << vF.width() << "/" << fullWidth
                         << "; actual/full height: " << vF.height() << "/" << fullHeight
                         << "; UV scale: U " << uScale << " V " << vScale;
#endif

                GLuint physical = ~0U;
#if GST_CHECK_VERSION(1,14,0)
                auto buffer = reinterpret_cast<QGstVideoBuffer *>(vF.buffer());
                auto mem = gst_buffer_peek_memory(buffer->buffer(), 0);
                auto phys_addr = gst_is_phys_memory(mem) ? gst_phys_memory_get_phys_addr(mem) : 0;
                if (phys_addr)
                    physical = phys_addr;
#endif
                glBindTexture(GL_TEXTURE_2D, tmpTexId);
                glTexDirectVIVMap_LOCAL(GL_TEXTURE_2D,
                                        fullWidth, fullHeight,
                                        QSGVivanteVideoNode::getVideoFormat2GLFormatMap().value(vF.pixelFormat()),
                                        &bits, &physical);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexDirectInvalidateVIV_LOCAL(GL_TEXTURE_2D);

                mMapError = glGetError();
                if (mMapError == GL_NO_ERROR)
                    return tmpTexId;

                // Error occurred.
                // Fallback to copying data.
            } else {
                // Fastest path: already seen this logical address. Just
                // indicate that the data belonging to the texture has changed.
                glBindTexture(GL_TEXTURE_2D, mBitsToTextureMap.value(vF.bits()));
                glTexDirectInvalidateVIV_LOCAL(GL_TEXTURE_2D);
                return mBitsToTextureMap.value(vF.bits());
            }
        }

        // Cannot map. So copy.
        if (!mTexDirectTexture) {
            glGenTextures(1, &mTexDirectTexture);
            glBindTexture(GL_TEXTURE_2D, mTexDirectTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexDirectVIV_LOCAL(GL_TEXTURE_2D, mCurrentFrame.width(), mCurrentFrame.height(),
                                 QSGVivanteVideoNode::getVideoFormat2GLFormatMap().value(mCurrentFrame.pixelFormat()),
                                 (GLvoid **) &mTexDirectPlanes);
        } else {
            glBindTexture(GL_TEXTURE_2D, mTexDirectTexture);
        }
        switch (mCurrentFrame.pixelFormat()) {
        case QVideoFrameFormat::Format_YUV420P:
        case QVideoFrameFormat::Format_YV12:
            memcpy(mTexDirectPlanes[0], mCurrentFrame.bits(0), mCurrentFrame.height() * mCurrentFrame.bytesPerLine(0));
            memcpy(mTexDirectPlanes[1], mCurrentFrame.bits(1), mCurrentFrame.height() / 2 * mCurrentFrame.bytesPerLine(1));
            memcpy(mTexDirectPlanes[2], mCurrentFrame.bits(2), mCurrentFrame.height() / 2 * mCurrentFrame.bytesPerLine(2));
            break;
        case QVideoFrameFormat::Format_NV12:
        case QVideoFrameFormat::Format_NV21:
            memcpy(mTexDirectPlanes[0], mCurrentFrame.bits(0), mCurrentFrame.height() * mCurrentFrame.bytesPerLine(0));
            memcpy(mTexDirectPlanes[1], mCurrentFrame.bits(1), mCurrentFrame.height() / 2 * mCurrentFrame.bytesPerLine(1));
            break;
        default:
            memcpy(mTexDirectPlanes[0], mCurrentFrame.bits(), mCurrentFrame.height() * mCurrentFrame.bytesPerLine());
            break;
        }
        glTexDirectInvalidateVIV_LOCAL(GL_TEXTURE_2D);
        return mTexDirectTexture;
    }
    else {
#ifdef QT_VIVANTE_VIDEO_DEBUG
        qWarning() << " couldn't map the QVideoFrame vF: " << vF;
#endif
        return 0;
    }

    Q_ASSERT(false); // should never reach this line!;
    return 0;
}

/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/qx11info_x11.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>
#include <qvideosurfaceformat.h>

#include "qxvideosurface_maemo5_p.h"

//#define DEBUG_XV_SURFACE

struct XvFormatRgb
{
    QVideoFrame::PixelFormat pixelFormat;
    int bits_per_pixel;
    int format;
    int num_planes;

    int depth;
    unsigned int red_mask;
    unsigned int green_mask;
    unsigned int blue_mask;

};

bool operator ==(const XvImageFormatValues &format, const XvFormatRgb &rgb)
{
    return format.type == XvRGB
            && format.bits_per_pixel == rgb.bits_per_pixel
            && format.format         == rgb.format
            && format.num_planes     == rgb.num_planes
            && format.depth          == rgb.depth
            && format.red_mask       == rgb.red_mask
            && format.blue_mask      == rgb.blue_mask;
}

static const XvFormatRgb qt_xvRgbLookup[] =
{
    { QVideoFrame::Format_ARGB32, 32, XvPacked, 1, 32, 0x00FF0000, 0x0000FF00, 0x000000FF },
    { QVideoFrame::Format_RGB32 , 32, XvPacked, 1, 24, 0x00FF0000, 0x0000FF00, 0x000000FF },
    { QVideoFrame::Format_RGB24 , 24, XvPacked, 1, 24, 0x00FF0000, 0x0000FF00, 0x000000FF },
    { QVideoFrame::Format_RGB565, 16, XvPacked, 1, 16, 0x0000F800, 0x000007E0, 0x0000001F },
    { QVideoFrame::Format_BGRA32, 32, XvPacked, 1, 32, 0xFF000000, 0x00FF0000, 0x0000FF00 },
    { QVideoFrame::Format_BGR32 , 32, XvPacked, 1, 24, 0x00FF0000, 0x0000FF00, 0x000000FF },
    { QVideoFrame::Format_BGR24 , 24, XvPacked, 1, 24, 0x00FF0000, 0x0000FF00, 0x000000FF },
    { QVideoFrame::Format_BGR565, 16, XvPacked, 1, 16, 0x0000F800, 0x000007E0, 0x0000001F }
};

struct XvFormatYuv
{
    QVideoFrame::PixelFormat pixelFormat;
    int bits_per_pixel;
    int format;
    int num_planes;

    unsigned int y_sample_bits;
    unsigned int u_sample_bits;
    unsigned int v_sample_bits;
    unsigned int horz_y_period;
    unsigned int horz_u_period;
    unsigned int horz_v_period;
    unsigned int vert_y_period;
    unsigned int vert_u_period;
    unsigned int vert_v_period;
    char component_order[32];
};

bool operator ==(const XvImageFormatValues &format, const XvFormatYuv &yuv)
{
    return format.type == XvYUV
            && format.bits_per_pixel == yuv.bits_per_pixel
            && format.format         == yuv.format
            && format.num_planes     == yuv.num_planes
            && format.y_sample_bits  == yuv.y_sample_bits
            && format.u_sample_bits  == yuv.u_sample_bits
            && format.v_sample_bits  == yuv.v_sample_bits
            && format.horz_y_period  == yuv.horz_y_period
            && format.horz_u_period  == yuv.horz_u_period
            && format.horz_v_period  == yuv.horz_v_period
            && format.horz_y_period  == yuv.vert_y_period
            && format.vert_u_period  == yuv.vert_u_period
            && format.vert_v_period  == yuv.vert_v_period
            && qstrncmp(format.component_order, yuv.component_order, 32) == 0;
}

static const XvFormatYuv qt_xvYuvLookup[] =
{
    { QVideoFrame::Format_YUV444 , 24, XvPacked, 1, 8, 8, 8, 1, 1, 1, 1, 1, 1, "YUV"  },
    { QVideoFrame::Format_YUV420P, 12, XvPlanar, 3, 8, 8, 8, 1, 2, 2, 1, 2, 2, "YUV"  },
    { QVideoFrame::Format_YV12   , 12, XvPlanar, 3, 8, 8, 8, 1, 2, 2, 1, 2, 2, "YVU"  },
    { QVideoFrame::Format_UYVY   , 16, XvPacked, 1, 8, 8, 8, 1, 2, 2, 1, 1, 1, "UYVY" },
    { QVideoFrame::Format_YUYV   , 16, XvPacked, 1, 8, 8, 8, 1, 2, 2, 1, 1, 1, "YUY2" },
    { QVideoFrame::Format_YUYV   , 16, XvPacked, 1, 8, 8, 8, 1, 2, 2, 1, 1, 1, "YUYV" },
    { QVideoFrame::Format_NV12   , 12, XvPlanar, 2, 8, 8, 8, 1, 2, 2, 1, 2, 2, "YUV"  },
    { QVideoFrame::Format_NV12   , 12, XvPlanar, 2, 8, 8, 8, 1, 2, 2, 1, 2, 2, "YVU"  },
    { QVideoFrame::Format_Y8     , 8 , XvPlanar, 1, 8, 0, 0, 1, 0, 0, 1, 0, 0, "Y"    }
};

QXVideoSurface::QXVideoSurface(QObject *parent)
    : QAbstractVideoSurface(parent)
    , m_winId(0)
    , m_portId(0)
    , m_gc(0)
    , m_image(0)
    , m_colorKey(24,0,24)
{
}

QXVideoSurface::~QXVideoSurface()
{
    if (m_gc)
        XFreeGC(QX11Info::display(), m_gc);

    if (m_portId != 0)
        XvUngrabPort(QX11Info::display(), m_portId, 0);
}

WId QXVideoSurface::winId() const
{
    return m_winId;
}

void QXVideoSurface::setWinId(WId id)
{
    if (id == m_winId)
        return;

#ifdef DEBUG_XV_SURFACE
    qDebug() << "QXVideoSurface::setWinId" << id;
#endif

    if (m_image)
        XFree(m_image);

    if (m_gc) {
        XFreeGC(QX11Info::display(), m_gc);
        m_gc = 0;
    }

    if (m_portId != 0)
        XvUngrabPort(QX11Info::display(), m_portId, 0);

    QList<QVideoFrame::PixelFormat> prevFormats = m_supportedPixelFormats;
    m_supportedPixelFormats.clear();
    m_formatIds.clear();

    m_winId = id;

    if (m_winId && findPort()) {
        querySupportedFormats();

        m_gc = XCreateGC(QX11Info::display(), m_winId, 0, 0);

        if (m_image) {
            m_image = 0;

            if (!start(surfaceFormat()))
                QAbstractVideoSurface::stop();
        }
    } else if (m_image) {
        m_image = 0;

        QAbstractVideoSurface::stop();
    }

    if (m_supportedPixelFormats != prevFormats) {
#ifdef DEBUG_XV_SURFACE
        qDebug() << "QXVideoSurface: supportedFormatsChanged";
#endif
        emit supportedFormatsChanged();
    }
}

QRect QXVideoSurface::displayRect() const
{
    return m_displayRect;
}

void QXVideoSurface::setDisplayRect(const QRect &rect)
{
    m_displayRect = rect;
}

QColor QXVideoSurface::colorKey() const
{
    return m_colorKey;
}

void QXVideoSurface::setColorKey(QColor key)
{
    m_colorKey = key;
}

int QXVideoSurface::getAttribute(const char *attribute) const
{
    if (m_portId != 0) {
        Display *display = QX11Info::display();

        Atom atom = XInternAtom(display, attribute, True);

        int value = 0;

        XvGetPortAttribute(display, m_portId, atom, &value);

        return value;
    } else {
        return 0;
    }
}

void QXVideoSurface::setAttribute(const char *attribute, int value)
{
    if (m_portId != 0) {
        Display *display = QX11Info::display();

        Atom atom = XInternAtom(display, attribute, True);

        XvSetPortAttribute(display, m_portId, atom, value);
    }
}

QList<QVideoFrame::PixelFormat> QXVideoSurface::supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType) const
{
    if ( handleType == QAbstractVideoBuffer::NoHandle ||
         handleType == QAbstractVideoBuffer::XvShmImageHandle )
        return m_supportedPixelFormats;
    else
        return QList<QVideoFrame::PixelFormat>();
}

bool QXVideoSurface::start(const QVideoSurfaceFormat &format)
{
#ifdef DEBUG_XV_SURFACE
    qDebug() << "QXVideoSurface::start" << format;
#endif

    m_lastFrame = QVideoFrame();

    if (m_image)
        XFree(m_image);

    m_xvFormatId = 0;
    for (int i = 0; i < m_supportedPixelFormats.count(); ++i) {
        if (m_supportedPixelFormats.at(i) == format.pixelFormat()) {
            m_xvFormatId = m_formatIds.at(i);
            break;
        }
    }

    if (m_xvFormatId == 0) {
        setError(UnsupportedFormatError);
    } else {
        XvImage *image = XvShmCreateImage(
                QX11Info::display(),
                m_portId,
                m_xvFormatId,
                0,
                format.frameWidth(),
                format.frameHeight(),
                &m_shminfo
                );

        if (!image) {
            setError(ResourceError);
            return false;
        }

        m_shminfo.shmid = shmget(IPC_PRIVATE, image->data_size, IPC_CREAT | 0777);
        m_shminfo.shmaddr = image->data = (char*)shmat(m_shminfo.shmid, 0, 0);
        m_shminfo.readOnly = False;

        if (!XShmAttach(QX11Info::display(), &m_shminfo)) {
            qWarning() << "XShmAttach failed" << format;
            return false;
        }

        if (!image) {
            setError(ResourceError);
        } else {
            m_viewport = format.viewport();
            m_image = image;

            quint32 c = m_colorKey.rgb();
            quint16 colorKey16 = ((c >> 3) & 0x001f)
                   | ((c >> 5) & 0x07e0)
                   | ((c >> 8) & 0xf800);

            setAttribute("XV_AUTOPAINT_COLORKEY", 0);
            setAttribute("XV_COLORKEY", colorKey16);
            setAttribute("XV_OMAP_VSYNC", 1);
            setAttribute("XV_DOUBLE_BUFFER", 0);

            QVideoSurfaceFormat newFormat = format;
            newFormat.setProperty("portId", QVariant(quint64(m_portId)));
            newFormat.setProperty("xvFormatId", m_xvFormatId);
            newFormat.setProperty("dataSize", image->data_size);

            return QAbstractVideoSurface::start(newFormat);
        }
    }

    if (m_image) {
        m_image = 0;

        QAbstractVideoSurface::stop();
    }

    return false;
}

void QXVideoSurface::stop()
{
    if (m_image) {
        XFree(m_image);
        m_image = 0;
        m_lastFrame = QVideoFrame();

        QAbstractVideoSurface::stop();
    }
}

bool QXVideoSurface::present(const QVideoFrame &frame)
{
    if (!m_image) {
        setError(StoppedError);
        return false;
    } else if (m_image->width != frame.width() || m_image->height != frame.height()) {
        setError(IncorrectFormatError);
        return false;
    } else {
        m_lastFrame = frame;

        if (!m_lastFrame.map(QAbstractVideoBuffer::ReadOnly)) {
            qWarning() << "Failed to map video frame";
            setError(IncorrectFormatError);
            return false;
        } else {
            bool presented = false;

            if (frame.handleType() != QAbstractVideoBuffer::XvShmImageHandle &&
                m_image->data_size > m_lastFrame.mappedBytes()) {
                qWarning("Insufficient frame buffer size");
                setError(IncorrectFormatError);
            } else if (frame.handleType() != QAbstractVideoBuffer::XvShmImageHandle &&
                       m_image->num_planes > 0 &&
                       m_image->pitches[0] != m_lastFrame.bytesPerLine()) {
                qWarning("Incompatible frame pitches");
                setError(IncorrectFormatError);
            } else {
                XvImage *img = 0;

                if (frame.handleType() == QAbstractVideoBuffer::XvShmImageHandle) {
                    img = frame.handle().value<XvImage*>();
                } else {
                    img = m_image;
                    memcpy(m_image->data, m_lastFrame.bits(), qMin(m_lastFrame.mappedBytes(), m_image->data_size));
                }

                if (img)
                    XvShmPutImage(
                       QX11Info::display(),
                       m_portId,
                       m_winId,
                       m_gc,
                       img,
                       m_viewport.x(),
                       m_viewport.y(),
                       m_viewport.width(),
                       m_viewport.height(),
                       m_displayRect.x(),
                       m_displayRect.y(),
                       m_displayRect.width(),
                       m_displayRect.height(),
                       false);

                presented = true;
            }

            m_lastFrame.unmap();

            return presented;
        }
    }
}

void QXVideoSurface::repaintLastFrame()
{
    if (m_lastFrame.isValid())
        present(QVideoFrame(m_lastFrame));
}

bool QXVideoSurface::findPort()
{
    unsigned int count = 0;
    XvAdaptorInfo *adaptors = 0;
    bool portFound = false;

    if (XvQueryAdaptors(QX11Info::display(), m_winId, &count, &adaptors) == Success) {
        for (unsigned int i = 0; i < count && !portFound; ++i) {
            if (adaptors[i].type & XvImageMask) {
                m_portId = adaptors[i].base_id;

                for (unsigned int j = 0; j < adaptors[i].num_ports && !portFound; ++j, ++m_portId)
                    portFound = XvGrabPort(QX11Info::display(), m_portId, 0) == Success;
            }
        }
        XvFreeAdaptorInfo(adaptors);
    }    

    if (!portFound)
        qWarning() << "QXVideoSurface::findPort: failed to find XVideo port";

    return portFound;
}

void QXVideoSurface::querySupportedFormats()
{
    int count = 0;
    if (XvImageFormatValues *imageFormats = XvListImageFormats(
            QX11Info::display(), m_portId, &count)) {
        const int rgbCount = sizeof(qt_xvRgbLookup) / sizeof(XvFormatRgb);
        const int yuvCount = sizeof(qt_xvYuvLookup) / sizeof(XvFormatYuv);

        for (int i = 0; i < count; ++i) {
            switch (imageFormats[i].type) {
            case XvRGB:
                for (int j = 0; j < rgbCount; ++j) {
                    if (imageFormats[i] == qt_xvRgbLookup[j]) {
                        m_supportedPixelFormats.append(qt_xvRgbLookup[j].pixelFormat);
                        m_formatIds.append(imageFormats[i].id);
                        break;
                    }
                }
                break;
            case XvYUV:
                for (int j = 0; j < yuvCount; ++j) {
                    //skip YUV420P and YV12 formats, they don't work correctly and slow,
                    //YUV2 == YUYV is just slow
                    if (imageFormats[i] == qt_xvYuvLookup[j] &&
                        qt_xvYuvLookup[j].pixelFormat != QVideoFrame::Format_YUV420P &&
                        qt_xvYuvLookup[j].pixelFormat != QVideoFrame::Format_YV12) {
                        m_supportedPixelFormats.append(qt_xvYuvLookup[j].pixelFormat);
                        m_formatIds.append(imageFormats[i].id);
                        break;
                    }
                }
                break;
            }
        }
        XFree(imageFormats);
    }

#ifdef DEBUG_XV_SURFACE
    qDebug() << "Supported pixel formats:" << m_supportedPixelFormats;
#endif

}

/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGSTBUFFERPOOLINTERFACE_P_H
#define QGSTBUFFERPOOLINTERFACE_P_H

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

#include <qabstractvideobuffer.h>
#include <qvideosurfaceformat.h>
#include <QtCore/qobject.h>
#include <QtCore/qplugin.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

const QLatin1String QGstBufferPoolPluginKey("bufferpool");

/*!
    Abstract interface for video buffers allocation.
*/
class QGstBufferPoolInterface
{
public:
    virtual ~QGstBufferPoolInterface() {}

    virtual bool isFormatSupported(const QVideoSurfaceFormat &format) const = 0;

    virtual GType bufferType() const = 0;
    virtual GstBuffer *takeBuffer(const QVideoSurfaceFormat &format, GstCaps *caps) = 0;
    virtual void clear() = 0;

    virtual QAbstractVideoBuffer::HandleType handleType() const = 0;

    /*!
      Build an QAbstractVideoBuffer instance from compatible (mathcing gst buffer type)
      GstBuffer.

      This method is called from gstreamer video sink thread.
     */
    virtual QAbstractVideoBuffer *prepareVideoBuffer(GstBuffer *buffer, int bytesPerLine) = 0;
};

#define QGstBufferPoolInterface_iid "com.nokia.Qt.QGstBufferPoolInterface"
Q_DECLARE_INTERFACE(QGstBufferPoolInterface, QGstBufferPoolInterface_iid)

class QGstBufferPoolPlugin : public QObject, public QGstBufferPoolInterface
{
    Q_OBJECT
    Q_INTERFACES(QGstBufferPoolInterface)
public:
    explicit QGstBufferPoolPlugin(QObject *parent = 0);
    virtual ~QGstBufferPoolPlugin() {}

    virtual bool isFormatSupported(const QVideoSurfaceFormat &format) const = 0;

    virtual GType bufferType() const = 0;
    virtual GstBuffer *takeBuffer(const QVideoSurfaceFormat &format, GstCaps *caps) = 0;
    virtual void clear() = 0;

    virtual QAbstractVideoBuffer::HandleType handleType() const = 0;

    /*!
      Build an QAbstractVideoBuffer instance from compatible (mathcing gst buffer type)
      GstBuffer.

      This method is called from gstreamer video sink thread.
     */
    virtual QAbstractVideoBuffer *prepareVideoBuffer(GstBuffer *buffer, int bytesPerLine) = 0;
};

QT_END_NAMESPACE

#endif

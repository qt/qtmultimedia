/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT7MOVIERENDERER_H
#define QT7MOVIERENDERER_H

#include "qt7backend.h"

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>

#include <qvideorenderercontrol.h>
#include <qmediaplayer.h>

#include "qt7videooutput.h"

#include <QuartzCore/CVOpenGLTexture.h>
#include <QuickTime/QuickTime.h>

QT_BEGIN_NAMESPACE

class QGLContext;

class QCvDisplayLink;
class QT7PlayerSession;
class QT7PlayerService;

class QT7MovieRenderer : public QT7VideoRendererControl
{
Q_OBJECT
public:
    QT7MovieRenderer(QObject *parent = 0);
    virtual ~QT7MovieRenderer();

    void setMovie(void *movie);
    void updateNaturalSize(const QSize &newSize);

    QAbstractVideoSurface *surface() const;
    void setSurface(QAbstractVideoSurface *surface);

    QSize nativeSize() const;

private Q_SLOTS:
    void updateVideoFrame(const CVTimeStamp &ts);

private:
    void setupVideoOutput();
    bool createPixelBufferVisualContext();
    bool createGLVisualContext();

    void *m_movie;

    QMutex m_mutex;

    QCvDisplayLink *m_displayLink;
#ifdef QUICKTIME_C_API_AVAILABLE
    QTVisualContextRef m_visualContext;
    bool m_usingGLContext;
    const QGLContext *m_currentGLContext;
    QSize m_pixelBufferContextGeometry;
#endif
    QAbstractVideoSurface *m_surface;
    QSize m_nativeSize;
};

QT_END_NAMESPACE

#endif

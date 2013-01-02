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

#ifndef QT7MOVIEVIEWRENDERER_H
#define QT7MOVIEVIEWRENDERER_H

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>

#include <qvideowindowcontrol.h>
#include <qmediaplayer.h>

#include "qt7videooutput.h"
#include <qvideoframe.h>

#include <QuartzCore/CIContext.h>

QT_BEGIN_NAMESPACE

class QVideoFrame;

class QT7PlayerSession;
class QT7PlayerService;
class QGLWidget;
class QGLFramebufferObject;
class QWindow;
class QOpenGLContext;

class QT7MovieViewRenderer : public QT7VideoRendererControl
{
public:
    QT7MovieViewRenderer(QObject *parent = 0);
    ~QT7MovieViewRenderer();

    void setMovie(void *movie);
    void updateNaturalSize(const QSize &newSize);

    QAbstractVideoSurface *surface() const;
    void setSurface(QAbstractVideoSurface *surface);

    void renderFrame(const QVideoFrame &);

protected:
    bool event(QEvent *event);

private:
    void setupVideoOutput();
    QVideoFrame convertCIImageToGLTexture(const QVideoFrame &frame);

    void *m_movie;
    void *m_movieView;
    QSize m_nativeSize;
    QAbstractVideoSurface *m_surface;
    QVideoFrame m_currentFrame;
    QWindow *m_window;
    QOpenGLContext *m_context;
    QGLFramebufferObject *m_fbo;
    CIContext *m_ciContext;

    bool m_pendingRenderEvent;
    QMutex m_mutex;
};

QT_END_NAMESPACE

#endif

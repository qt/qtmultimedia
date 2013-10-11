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

#ifndef QANDROIDVIDEORENDERCONTROL_H
#define QANDROIDVIDEORENDERCONTROL_H

#include <qvideorenderercontrol.h>
#include "qandroidvideooutput.h"
#include "jsurfacetexture.h"

QT_BEGIN_NAMESPACE

class JSurfaceTextureHolder;

class TextureDeleter : public QObject
{
    Q_OBJECT
public:
    TextureDeleter(uint id) : m_id(id) { }
    ~TextureDeleter();

private:
    uint m_id;
};

class QAndroidVideoRendererControl : public QVideoRendererControl, public QAndroidVideoOutput
{
    Q_OBJECT
    Q_INTERFACES(QAndroidVideoOutput)
public:
    explicit QAndroidVideoRendererControl(QObject *parent = 0);
    ~QAndroidVideoRendererControl() Q_DECL_OVERRIDE;

    QAbstractVideoSurface *surface() const Q_DECL_OVERRIDE;
    void setSurface(QAbstractVideoSurface *surface) Q_DECL_OVERRIDE;

    jobject surfaceHolder() Q_DECL_OVERRIDE;
    jobject surfaceTexture() Q_DECL_OVERRIDE;
    bool isReady() Q_DECL_OVERRIDE;
    void setVideoSize(const QSize &size) Q_DECL_OVERRIDE;
    void stop() Q_DECL_OVERRIDE;

    void customEvent(QEvent *) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void readyChanged(bool);

private Q_SLOTS:
    void onFrameAvailable();

private:
    bool initSurfaceTexture();

    QAbstractVideoSurface *m_surface;
    QSize m_nativeSize;

    QJNIObjectPrivate *m_androidSurface;
    JSurfaceTexture *m_surfaceTexture;
    JSurfaceTextureHolder *m_surfaceHolder;
    uint m_externalTex;
    TextureDeleter *m_textureDeleter;
};

QT_END_NAMESPACE

#endif // QANDROIDVIDEORENDERCONTROL_H

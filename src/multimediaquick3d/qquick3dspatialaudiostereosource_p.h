/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Spatial Audio module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-NOGPL2$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QQUICK3DSPATIALAUDIOSTEREOSOUND_H
#define QQUICK3DSPATIALAUDIOSTEREOSOUND_H

#include <private/qquick3dnode_p.h>
#include <QUrl>
#include <qvector3d.h>

QT_BEGIN_NAMESPACE

class QSpatialAudioStereoSource;

class QQuick3DSpatialAudioStereoSource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    QML_NAMED_ELEMENT(SpatialAudioStereoSource)

public:
    QQuick3DSpatialAudioStereoSource();
    ~QQuick3DSpatialAudioStereoSource();

    void setSource(QUrl source);
    QUrl source() const;

    void setVolume(float volume);
    float volume() const;

Q_SIGNALS:
    void sourceChanged();
    void volumeChanged();

private:
    QSpatialAudioStereoSource *m_sound = nullptr;
};

QT_END_NAMESPACE

#endif

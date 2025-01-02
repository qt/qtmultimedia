// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only
#ifndef QQUICK3DLISTENER_H
#define QQUICK3DLISTENER_H

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

#include <private/qquick3dnode_p.h>
#include <QtGui/qvector3d.h>

#include <qaudiolistener.h>

QT_BEGIN_NAMESPACE

class QQuick3DAudioListener : public QQuick3DNode
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AudioListener)

public:
    QQuick3DAudioListener();
    ~QQuick3DAudioListener() override;

    QAudioListener *listener() { return m_listener; }
protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *) override { return nullptr; }

protected Q_SLOTS:
    void updatePosition();
    void updateRotation();

private:
    QAudioListener *m_listener;
};

QT_END_NAMESPACE

#endif

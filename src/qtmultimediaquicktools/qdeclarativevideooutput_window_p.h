/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QDECLARATIVEVIDEOOUTPUT_WINDOW_P_H
#define QDECLARATIVEVIDEOOUTPUT_WINDOW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qdeclarativevideooutput_backend_p.h>

QT_BEGIN_NAMESPACE

class QVideoWindowControl;

class QDeclarativeVideoWindowBackend : public QDeclarativeVideoBackend
{
public:
    QDeclarativeVideoWindowBackend(QDeclarativeVideoOutput *parent);
    ~QDeclarativeVideoWindowBackend();

    bool init(QMediaService *service) override;
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &changeData) override;
    void releaseSource() override;
    void releaseControl() override;
    QSize nativeSize() const override;
    void updateGeometry() override;
    QSGNode *updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *data) override;
    QAbstractVideoSurface *videoSurface() const override;
    QRectF adjustedViewport() const override;

private:
    QPointer<QVideoWindowControl> m_videoWindowControl;
    bool m_visible;
};

QT_END_NAMESPACE

#endif

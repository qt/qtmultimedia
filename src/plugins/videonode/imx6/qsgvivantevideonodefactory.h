// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QSGVIDEONODEFACTORY_VIVANTE_H
#define QSGVIDEONODEFACTORY_VIVANTE_H

#include <QObject>
#include <private/qsgvideonode_p.h>

class QSGVivanteVideoNodeFactory : public QObject, public QSGVideoNodeFactoryInterface
{
public:
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QSGVideoNodeFactoryInterface_iid FILE "imx6.json")
    Q_INTERFACES(QSGVideoNodeFactoryInterface)

    QList<QVideoFrameFormat::PixelFormat> supportedPixelFormats(QVideoFrame::HandleType handleType) const;
    QSGVideoNode *createNode(const QVideoFrameFormat &format);
};
#endif // QSGVIDEONODEFACTORY_VIVANTE_H

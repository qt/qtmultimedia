/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QSGVIDEOTEXTURE_H
#define QSGVIDEOTEXTURE_H

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

#include <QtQuick/QSGTexture>
#include <QImage>
#include <private/qrhi_p.h>
#include <private/qtmultimediaquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QSGVideoTexturePrivate;
class Q_MULTIMEDIAQUICK_EXPORT QSGVideoTexture : public QSGTexture
{
    Q_DECLARE_PRIVATE(QSGVideoTexture)
public:
    QSGVideoTexture();
    ~QSGVideoTexture();

    qint64 comparisonKey() const override;
    QRhiTexture *rhiTexture() const override;
    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;
    void commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates) override;
    void setRhiTexture(QRhiTexture *texture);
    void setData(QRhiTexture::Format f, const QSize &s, const uchar *data, int bytes);
    void setNativeObject(quint64 obj, const QSize &s, QRhiTexture::Format f = QRhiTexture::RGBA8);

protected:
    QScopedPointer<QSGVideoTexturePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QSGVIDEOTEXTURE_H

/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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


#ifndef QGSTREAMERV4L2INPUT_H
#define QGSTREAMERV4L2INPUT_H

#include <QtCore/qhash.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qsize.h>
#include "qgstreamercapturesession.h"

QT_BEGIN_NAMESPACE

class QGstreamerV4L2Input : public QObject, public QGstreamerVideoInput
{
    Q_OBJECT
public:
    QGstreamerV4L2Input(QObject *parent = 0);
    virtual ~QGstreamerV4L2Input();

    GstElement *buildElement() override;

    QList<qreal> supportedFrameRates(const QSize &frameSize = QSize()) const override;
    QList<QSize> supportedResolutions(qreal frameRate = -1) const override;

    QByteArray device() const;

public slots:
    void setDevice(const QByteArray &device);
    void setDevice(const QString &device);

private:
    void updateSupportedResolutions(const QByteArray &device);

    QList<qreal> m_frameRates;
    QList<QSize> m_resolutions;

    QHash<QSize, QSet<int> > m_ratesByResolution;

    QByteArray m_device;
};

QT_END_NAMESPACE

#endif // QGSTREAMERV4L2INPUT_H

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

#ifndef QABSTRACTVIDEOFILTER_H
#define QABSTRACTVIDEOFILTER_H

#include <QtCore/qobject.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideosurfaceformat.h>

QT_BEGIN_NAMESPACE

class QAbstractVideoFilterPrivate;

class Q_MULTIMEDIA_EXPORT QVideoFilterRunnable
{
public:
    enum RunFlag {
        LastInChain = 0x01
    };
    Q_DECLARE_FLAGS(RunFlags, RunFlag)

    virtual ~QVideoFilterRunnable();
    virtual QVideoFrame run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags) = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QVideoFilterRunnable::RunFlags)

class Q_MULTIMEDIA_EXPORT QAbstractVideoFilter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)

public:
    explicit QAbstractVideoFilter(QObject *parent = nullptr);
    ~QAbstractVideoFilter();

    bool isActive() const;
    void setActive(bool v);

    virtual QVideoFilterRunnable *createFilterRunnable() = 0;

Q_SIGNALS:
    void activeChanged();

private:
    Q_DECLARE_PRIVATE(QAbstractVideoFilter)
    Q_DISABLE_COPY(QAbstractVideoFilter)

    QAbstractVideoFilterPrivate *d_ptr;
};

QT_END_NAMESPACE

#endif // QABSTRACTVIDEOFILTER_H

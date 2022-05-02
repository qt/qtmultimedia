/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QQUICKSOUNDEFFECT_H
#define QQUICKSOUNDEFFECT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QSoundEffect>
#include <QtQml/qqml.h>
#include <QtQml/qqmlcontext.h>
#include <private/qtmultimediaquickglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIAQUICK_EXPORT QQuickSoundEffect : public QSoundEffect
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ qmlSource WRITE qmlSetSource NOTIFY sourceChanged)
    QML_NAMED_ELEMENT(SoundEffect)

public:
    QQuickSoundEffect(QObject *parent = nullptr) : QSoundEffect(parent) {}

    void qmlSetSource(const QUrl &source)
    {
        if (m_source == source)
            return;

        m_source = source;
        const QQmlContext *context = qmlContext(this);
        setSource(context ? context->resolvedUrl(source) : source);
        emit sourceChanged(source);
    }

    QUrl qmlSource() const { return m_source; }

Q_SIGNALS:
    void sourceChanged(const QUrl &source);

private:
    QUrl m_source;
};

QT_END_NAMESPACE

#endif

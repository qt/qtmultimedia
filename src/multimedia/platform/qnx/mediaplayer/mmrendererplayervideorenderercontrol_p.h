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
******************************************************************************/
#ifndef MMRENDERERPLAYERVIDEORENDERERCONTROL_H
#define MMRENDERERPLAYERVIDEORENDERERCONTROL_H

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

#include <QPointer>
#include <qobject.h>

typedef struct mmr_context mmr_context_t;

QT_BEGIN_NAMESPACE

class WindowGrabber;
class QVideoSink;

class MmRendererPlayerVideoRendererControl : public QObject
{
    Q_OBJECT
public:
    explicit MmRendererPlayerVideoRendererControl(QObject *parent = 0);
    ~MmRendererPlayerVideoRendererControl();

    QVideoSink *sink() const;
    void setSink(QVideoSink *sink);

    // Called by media control
    void attachDisplay(mmr_context_t *context);
    void detachDisplay();
    void pause();
    void resume();

    void customEvent(QEvent *) override;

private Q_SLOTS:
    void updateScene(const QSize &size);

private:
    QPointer<QVideoSink> m_sink;

    WindowGrabber* m_windowGrabber;
    mmr_context_t *m_context;

    int m_videoId;
};

QT_END_NAMESPACE

#endif

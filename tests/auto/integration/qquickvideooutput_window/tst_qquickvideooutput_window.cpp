/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=plugins/declarative/multimedia

#include "private/qquickvideooutput_p.h"
#include <QtCore/qobject.h>
#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <private/qplatformvideosink_p.h>
#include <qmediaplayer.h>

class QtTestVideoObject : public QObject
{
    Q_OBJECT
public:
    explicit QtTestVideoObject()
        : QObject(nullptr)
    {
    }
};

class tst_QQuickVideoOutputWindow : public QObject
{
    Q_OBJECT
public:
    tst_QQuickVideoOutputWindow()
        : QObject(nullptr)
        , m_sourceObject(&m_videoObject)
    {
    }

    ~tst_QQuickVideoOutputWindow() override
    = default;

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void aspectRatio();

private:
    QQmlEngine m_engine;
    QQuickVideoOutput *m_videoItem;
    QScopedPointer<QQuickItem> m_rootItem;
    QtTestVideoObject m_videoObject;
    QMediaPlayer m_sourceObject;
    QQuickView m_view;
    QVideoSink *m_sink;
};

void tst_QQuickVideoOutputWindow::initTestCase()
{
    QQmlComponent component(&m_engine);
    component.loadUrl(QUrl("qrc:/main.qml"));

    m_rootItem.reset(qobject_cast<QQuickItem *>(component.create()));
    QVERIFY(m_rootItem != nullptr);
    m_videoItem = qobject_cast<QQuickVideoOutput *>(m_rootItem->findChild<QQuickItem *>("videoOutput"));
    QVERIFY(m_videoItem);
    m_sink = m_videoItem->videoSink();
    m_rootItem->setParentItem(m_view.contentItem());
    m_sourceObject.setVideoOutput(m_videoItem);

    m_view.resize(200, 200);
    m_view.show();
}

void tst_QQuickVideoOutputWindow::cleanupTestCase()
{
    // Make sure that QQuickVideoOutput doesn't segfault when it is being destroyed after
    // the service is already gone
    m_view.setSource(QUrl());
    m_rootItem.reset();
}

void tst_QQuickVideoOutputWindow::aspectRatio()
{
    m_videoItem->setProperty("fillMode", QQuickVideoOutput::Stretch);
    QTRY_COMPARE(m_videoItem->fillMode(), QQuickVideoOutput::Stretch);

    m_videoItem->setProperty("fillMode", QQuickVideoOutput::PreserveAspectFit);
    QTRY_COMPARE(m_videoItem->fillMode(), QQuickVideoOutput::PreserveAspectFit);

    m_videoItem->setProperty("fillMode", QQuickVideoOutput::PreserveAspectCrop);
    QTRY_COMPARE(m_videoItem->fillMode(), QQuickVideoOutput::PreserveAspectCrop);
}

QTEST_MAIN(tst_QQuickVideoOutputWindow)

#include "tst_qquickvideooutput_window.moc"

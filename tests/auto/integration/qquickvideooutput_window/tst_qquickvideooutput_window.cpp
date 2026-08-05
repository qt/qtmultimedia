// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//TESTED_COMPONENT=plugins/declarative/multimedia

#include <QtCore/qobject.h>

#include <QtMultimedia/qmediaplayer.h>

#include <QtMultimedia/private/qplatformvideosink_p.h>

#include <QtMultimediaQuick/private/qquickvideooutput_p.h>

#include <QtMultimediaTestLib/private/mediabackendutils_p.h>
#include <QtMultimediaTestLib/private/qintegrationtestbase_p.h>

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>

#include <QtTest/qtest.h>

#include <memory>

class QtTestVideoObject : public QObject
{
    Q_OBJECT
public:
    explicit QtTestVideoObject()
        : QObject(nullptr)
    {
    }
};

class tst_QQuickVideoOutputWindow : public QIntegrationTestBase
{
    Q_OBJECT
public:
    tst_QQuickVideoOutputWindow()
        : QIntegrationTestBase()
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
    std::unique_ptr<QQuickItem> m_rootItem;
    QtTestVideoObject m_videoObject;
    QMediaPlayer m_sourceObject;
    QQuickView m_view;
    QVideoSink *m_sink;
};

void tst_QQuickVideoOutputWindow::initTestCase()
{
    if (!initIntegrationTestCase())
        return;

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

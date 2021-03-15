/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "private/qdeclarativevideooutput_p.h"
#include <QtCore/qobject.h>
#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <private/qplatformvideosink_p.h>

class SourceObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *mediaSource READ mediaSource CONSTANT)
public:
    explicit SourceObject(QObject *mediaSource, QObject *parent = nullptr)
        : QObject(parent), m_mediaSource(mediaSource)
    {}

    [[nodiscard]] QObject *mediaSource() const
    { return m_mediaSource; }

private:
    QObject *m_mediaSource;
};

class QtTestWindowControl : public QPlatformVideoSink
{
public:
    [[nodiscard]] WId winId() const override { return m_winId; }
    void setWinId(WId id) override { m_winId = id; }

    [[nodiscard]] QRect displayRect() const override { return m_displayRect; }
    void setDisplayRect(const QRect &rect) override { m_displayRect = rect; }

    [[nodiscard]] bool isFullScreen() const override { return m_fullScreen; }
    void setFullScreen(bool fullScreen) override { emit fullScreenChanged(m_fullScreen = fullScreen); }

    [[nodiscard]] int repaintCount() const { return m_repaintCount; }
    void setRepaintCount(int count) { m_repaintCount = count; }
    void repaint() override { ++m_repaintCount; }

    [[nodiscard]] QSize nativeSize() const override { return m_nativeSize; }
    void setNativeSize(const QSize &size) { m_nativeSize = size; emit nativeSizeChanged(); }

    [[nodiscard]] Qt::AspectRatioMode aspectRatioMode() const override { return m_aspectRatioMode; }
    void setAspectRatioMode(Qt::AspectRatioMode mode) override { m_aspectRatioMode = mode; }

    [[nodiscard]] int brightness() const override { return m_brightness; }
    void setBrightness(int brightness) override { emit brightnessChanged(m_brightness = brightness); }

    [[nodiscard]] int contrast() const override { return m_contrast; }
    void setContrast(int contrast) override { emit contrastChanged(m_contrast = contrast); }

    [[nodiscard]] int hue() const override { return m_hue; }
    void setHue(int hue) override { emit hueChanged(m_hue = hue); }

    [[nodiscard]] int saturation() const override { return m_saturation; }
    void setSaturation(int saturation) override { emit saturationChanged(m_saturation = saturation); }

private:
    WId m_winId = 0;
    int m_repaintCount = 0;
    int m_brightness = 0;
    int m_contrast = 0;
    int m_hue = 0;
    int m_saturation = 0;
    Qt::AspectRatioMode m_aspectRatioMode = Qt::KeepAspectRatio;
    QRect m_displayRect;
    QSize m_nativeSize;
    bool m_fullScreen = false;
};

class QtTestVideoObject : public QObject
{
    Q_OBJECT
public:
    explicit QtTestVideoObject()
        : QObject(nullptr)
    {
    }
};

class tst_QDeclarativeVideoOutputWindow : public QObject
{
    Q_OBJECT
public:
    tst_QDeclarativeVideoOutputWindow()
        : QObject(nullptr)
        , m_sourceObject(&m_videoObject)
    {
    }

    ~tst_QDeclarativeVideoOutputWindow() override
    = default;

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void winId();
    void nativeSize();
    void aspectRatio();
    void geometryChange();
    void resetCanvas();

private:
    QQmlEngine m_engine;
    QQuickItem *m_videoItem;
    QScopedPointer<QQuickItem> m_rootItem;
    QtTestWindowControl m_windowControl;
    QtTestVideoObject m_videoObject;
    SourceObject m_sourceObject;
    QQuickView m_view;
};

void tst_QDeclarativeVideoOutputWindow::initTestCase()
{
    QQmlComponent component(&m_engine);
    component.loadUrl(QUrl("qrc:/main.qml"));

    m_rootItem.reset(qobject_cast<QQuickItem *>(component.create()));
    m_videoItem = m_rootItem->findChild<QQuickItem *>("videoOutput");
    QVERIFY(m_videoItem);
    m_rootItem->setParentItem(m_view.contentItem());
    m_videoItem->setProperty("source", QVariant::fromValue<QObject *>(&m_sourceObject));

    m_windowControl.setNativeSize(QSize(400, 200));
    m_view.resize(200, 200);
    m_view.show();
}

void tst_QDeclarativeVideoOutputWindow::cleanupTestCase()
{
    // Make sure that QDeclarativeVideoOutput doesn't segfault when it is being destroyed after
    // the service is already gone
    m_view.setSource(QUrl());
    m_rootItem.reset();
}

void tst_QDeclarativeVideoOutputWindow::winId()
{
    QCOMPARE(m_windowControl.winId(), m_view.winId());
}

void tst_QDeclarativeVideoOutputWindow::nativeSize()
{
    QCOMPARE(m_videoItem->implicitWidth(), qreal(400.0f));
    QCOMPARE(m_videoItem->implicitHeight(), qreal(200.0f));
}

void tst_QDeclarativeVideoOutputWindow::aspectRatio()
{
    const QRect expectedDisplayRect(25, 50, 150, 100);
    int oldRepaintCount = m_windowControl.repaintCount();
    m_videoItem->setProperty("fillMode", QDeclarativeVideoOutput::Stretch);
    QTRY_COMPARE(m_windowControl.aspectRatioMode(), Qt::IgnoreAspectRatio);
    QCOMPARE(m_windowControl.displayRect(), expectedDisplayRect);
    QVERIFY(m_windowControl.repaintCount() > oldRepaintCount);

    oldRepaintCount = m_windowControl.repaintCount();
    m_videoItem->setProperty("fillMode", QDeclarativeVideoOutput::PreserveAspectFit);
    QTRY_COMPARE(m_windowControl.aspectRatioMode(), Qt::KeepAspectRatio);
    QCOMPARE(m_windowControl.displayRect(), expectedDisplayRect);
    QVERIFY(m_windowControl.repaintCount() > oldRepaintCount);

    oldRepaintCount = m_windowControl.repaintCount();
    m_videoItem->setProperty("fillMode", QDeclarativeVideoOutput::PreserveAspectCrop);
    QTRY_COMPARE(m_windowControl.aspectRatioMode(), Qt::KeepAspectRatioByExpanding);
    QCOMPARE(m_windowControl.displayRect(), expectedDisplayRect);
    QVERIFY(m_windowControl.repaintCount() > oldRepaintCount);
}

void tst_QDeclarativeVideoOutputWindow::geometryChange()
{
    m_videoItem->setWidth(50);
    QTRY_COMPARE(m_windowControl.displayRect(), QRect(25, 50, 50, 100));

    m_videoItem->setX(30);
    QTRY_COMPARE(m_windowControl.displayRect(), QRect(30, 50, 50, 100));
}

void tst_QDeclarativeVideoOutputWindow::resetCanvas()
{
    m_rootItem->setParentItem(nullptr);
    QCOMPARE((int)m_windowControl.winId(), 0);
}


QTEST_MAIN(tst_QDeclarativeVideoOutputWindow)

#include "tst_qdeclarativevideooutput_window.moc"

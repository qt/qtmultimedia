// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "screencapturepreview.h"
#include "screenlistmodel.h"
#include "windowlistmodel.h"

#include <QMediaCaptureSession>
#include <QScreenCapture>
#include <QVideoWidget>

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>

#include <QGuiApplication>

ScreenCapturePreview::ScreenCapturePreview(QWidget *parent)
    : QWidget(parent),
      screenListView(new QListView(this)),
      windowListView(new QListView(this)),
      screenCapture(new QScreenCapture(this)),
      windows(QWindowCapture::capturableWindows()),
      mediaCaptureSession(new QMediaCaptureSession(this)),
      videoWidget(new QVideoWidget(this)),
      gridLayout(new QGridLayout(this)),
      startStopButton(new QPushButton(tr("Stop screencapture"), this)),
      screenLabel(new QLabel(tr("Double-click screen to capture:"), this)),
      windowLabel(new QLabel(tr("Double-click window to capture:"), this)),
      videoWidgetLabel(new QLabel(tr("QScreenCapture output:"), this))
{
    // Get lists of screens and windows:
    screenListModel = new ScreenListModel(this);
    windowListModel = new WindowListModel(windows, this);

    // Setup QScreenCapture with initial source:

    screenCapture->setScreen(QGuiApplication::primaryScreen());
    screenCapture->start();
    mediaCaptureSession->setScreenCapture(screenCapture);
    mediaCaptureSession->setVideoOutput(videoWidget);

    // Setup UI:

    screenListView->setModel(screenListModel);
    windowListView->setModel(windowListModel);

    gridLayout->addWidget(screenLabel, 0, 0);
    gridLayout->addWidget(screenListView, 1, 0);
    gridLayout->addWidget(windowLabel, 2, 0);
    gridLayout->addWidget(windowListView, 3, 0);
    gridLayout->addWidget(startStopButton, 4, 0);
    gridLayout->addWidget(videoWidgetLabel, 0, 1);
    gridLayout->addWidget(videoWidget, 1, 1, 4, 1);

    gridLayout->setColumnStretch(1, 1);
    gridLayout->setRowStretch(1, 1);
    gridLayout->setColumnMinimumWidth(0, 400);
    gridLayout->setColumnMinimumWidth(1, 400);
    gridLayout->setRowMinimumHeight(3, 1);

    connect(screenListView, &QAbstractItemView::activated, this, &ScreenCapturePreview::onScreenSelectionChanged);
    connect(windowListView, &QAbstractItemView::activated, this,
            &ScreenCapturePreview::onWindowSelectionChanged);
    connect(startStopButton, &QPushButton::clicked, this,
            &ScreenCapturePreview::onStartStopButtonClicked);
    connect(screenCapture, &QScreenCapture::errorOccurred, this, &ScreenCapturePreview::onScreenCaptureErrorOccured);
}

ScreenCapturePreview::~ScreenCapturePreview() = default;

void ScreenCapturePreview::onScreenSelectionChanged(QModelIndex index)
{
    screenCapture->setScreen(screenListModel->screen(index));
    screenSelected = true;
    updateCapture();
}

void ScreenCapturePreview::onWindowSelectionChanged(QModelIndex index)
{
    windowCapture->setWindow(windowListModel->window(index));
    screenSelected = false;
    updateCapture();
}

void ScreenCapturePreview::onScreenCaptureErrorOccured([[maybe_unused]] QScreenCapture::Error error,
                                                       const QString &errorString)
{
    QMessageBox::warning(this, "QScreenCapture: Error occurred", errorString);
}

void ScreenCapturePreview::onStartStopButtonClicked()
{
    started = !started;
    startStopButton->setText(started ? tr("Stop screencapture") : tr("Start screencapture"));
    updateCapture();
}

void ScreenCapturePreview::updateCapture()
{
    if (screenSelected) {
        if (started != screenCapture->isActive())
            screenCapture->setActive(started);
    } else {
        if (started != windowCapture->isActive())
            windowCapture->setActive(started);
    }
}

#include "moc_screencapturepreview.cpp"

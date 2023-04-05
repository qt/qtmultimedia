// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "screencapturepreview.h"
#include "screenlistmodel.h"

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
      screenCapture(new QScreenCapture(this)),
      mediaCaptureSession(new QMediaCaptureSession(this)),
      videoWidget(new QVideoWidget(this)),
      gridLayout(new QGridLayout(this)),
      startStopButton(new QPushButton("Stop screencapture", this)),
      screenLabel(new QLabel("Double-click screen to capture:", this)),
      videoWidgetLabel(new QLabel("QScreenCapture output:", this))
{
    // Get list of screens:
    screenListModel = new ScreenListModel(this);

    // Setup QScreenCapture with initial source:

    screenCapture->setScreen(QGuiApplication::primaryScreen());
    screenCapture->start();
    mediaCaptureSession->setScreenCapture(screenCapture);
    mediaCaptureSession->setVideoOutput(videoWidget);

    // Setup UI:

    screenListView->setModel(screenListModel);
    gridLayout->addWidget(screenLabel, 0, 0);
    gridLayout->addWidget(screenListView, 1, 0);
    gridLayout->addWidget(startStopButton, 2, 0);
    gridLayout->addWidget(videoWidgetLabel, 0, 1);
    gridLayout->addWidget(videoWidget, 1, 1, 2, 1);

    gridLayout->setColumnStretch(1, 1);
    gridLayout->setRowStretch(1, 1);
    gridLayout->setColumnMinimumWidth(0, 400);
    gridLayout->setColumnMinimumWidth(1, 400);

    connect(screenListView, &QAbstractItemView::activated, this, &ScreenCapturePreview::onScreenSelectionChanged);
    connect(startStopButton, &QPushButton::clicked, this, &ScreenCapturePreview::onStartStopButtonClicked);
    connect(screenCapture, &QScreenCapture::errorOccurred, this, &ScreenCapturePreview::onScreenCaptureErrorOccured);
}

ScreenCapturePreview::~ScreenCapturePreview() = default;

void ScreenCapturePreview::onScreenSelectionChanged(QModelIndex index)
{
    screenCapture->setScreen(screenListModel->screen(index));
}

void ScreenCapturePreview::onScreenCaptureErrorOccured([[maybe_unused]] QScreenCapture::Error error,
                                                       const QString &errorString)
{
    QMessageBox::warning(this, "QScreenCapture: Error occurred", errorString);
}

void ScreenCapturePreview::onStartStopButtonClicked()
{
    if (screenCapture->isActive()) {
        screenCapture->stop();
        startStopButton->setText(tr("Start screencapture"));
    } else {
        screenCapture->start();
        startStopButton->setText(tr("Stop screencapture"));
    }
}

#include "moc_screencapturepreview.cpp"

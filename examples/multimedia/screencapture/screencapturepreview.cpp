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
      windows(QGuiApplication::allWindows()),
      mediaCaptureSession(new QMediaCaptureSession(this)),
      videoWidget(new QVideoWidget(this)),
      gridLayout(new QGridLayout(this)),
      hBoxLayout(new QHBoxLayout),
      lineEdit(new QLineEdit(this)),
      wIdButton(new QPushButton("Confirm", this)),
      startStopButton(new QPushButton("Stop screencapture", this)),
      screenLabel(new QLabel("Double-click screen to capture:", this)),
      windowLabel(new QLabel("Double-click window to capture:", this)),
      windowIdLabel(new QLabel("Enter window ID: (e.g. 0x13C009E10 or 5301640720)", this)),
      videoWidgetLabel(new QLabel("QScreenCapture output:", this))
{
    // Get lists of screens and windows:
    screenListModel = new ScreenListModel(this);
    windowListModel = new WindowListModel(windows, this);
    qDebug() << "return value from QGuiApplication::allWindows(): " << windows;

    // Setup QScreenCapture with initial source:

    screenCapture->setScreen(QGuiApplication::primaryScreen());
    screenCapture->start();
    mediaCaptureSession->setScreenCapture(screenCapture);
    mediaCaptureSession->setVideoOutput(videoWidget);

    // Setup UI:

    screenListView->setModel(screenListModel);
    windowListView->setModel(windowListModel);
    // Sets initial lineEdit text to the WId of the second QWindow from QGuiApplication::allWindows()
    if (windows.size() > 1)
        lineEdit->setText(QString::number(windows[1]->winId()));

    gridLayout->addWidget(screenLabel, 0, 0);
    gridLayout->addWidget(screenListView, 1, 0);
    gridLayout->addWidget(windowLabel, 2, 0);
    gridLayout->addWidget(windowListView, 3, 0);
    gridLayout->addWidget(windowIdLabel, 4, 0);
    hBoxLayout->addWidget(lineEdit);
    hBoxLayout->addWidget(wIdButton);
    gridLayout->addLayout(hBoxLayout, 5, 0);
    gridLayout->addWidget(startStopButton, 6, 0);
    gridLayout->addWidget(videoWidgetLabel, 0, 1);
    gridLayout->addWidget(videoWidget, 1, 1, 6, 1);

    gridLayout->setColumnStretch(1, 1);
    gridLayout->setRowStretch(1, 1);
    gridLayout->setColumnMinimumWidth(0, 400);
    gridLayout->setColumnMinimumWidth(1, 400);
    gridLayout->setRowMinimumHeight(3, 1);

    connect(screenListView, &QAbstractItemView::activated, this, &ScreenCapturePreview::onScreenSelectionChanged);
    connect(windowListView, &QAbstractItemView::activated, this, &ScreenCapturePreview::onWindowSelectionChanged);
    connect(wIdButton, &QPushButton::clicked, this, &ScreenCapturePreview::onWindowIdSelectionChanged);
    connect(startStopButton, &QPushButton::clicked, this, &ScreenCapturePreview::onStartStopButtonClicked);
    connect(lineEdit, &QLineEdit::returnPressed, this, &ScreenCapturePreview::onWindowIdSelectionChanged);
    connect(screenCapture, &QScreenCapture::errorOccurred, this, &ScreenCapturePreview::onScreenCaptureErrorOccured);
}

ScreenCapturePreview::~ScreenCapturePreview() = default;

void ScreenCapturePreview::onScreenSelectionChanged(QModelIndex index)
{
    screenCapture->setScreen(screenListModel->screen(index));
}

void ScreenCapturePreview::onWindowSelectionChanged(QModelIndex index)
{
    screenCapture->setWindow(windowListModel->window(index));
}

void ScreenCapturePreview::onWindowIdSelectionChanged()
{
    unsigned long long input = lineEdit->text().trimmed().toULongLong(nullptr, 0);
    screenCapture->setWindowId(input);
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

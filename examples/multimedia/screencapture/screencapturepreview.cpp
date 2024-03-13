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
#include <QAction>

ScreenCapturePreview::ScreenCapturePreview(QWidget *parent)
    : QWidget(parent),
      screenListView(new QListView(this)),
      windowListView(new QListView(this)),
      screenCapture(new QScreenCapture(this)),
      windowCapture(new QWindowCapture(this)),
      mediaCaptureSession(new QMediaCaptureSession(this)),
      videoWidget(new QVideoWidget(this)),
      gridLayout(new QGridLayout(this)),
      startStopButton(new QPushButton(this)),
      screenLabel(new QLabel(tr("Select screen to capture:"), this)),
      windowLabel(new QLabel(tr("Select window to capture:"), this)),
      videoWidgetLabel(new QLabel(tr("Capture output:"), this))
{
    // Get lists of screens and windows:
    screenListModel = new ScreenListModel(this);
    windowListModel = new WindowListModel(this);

    // Setup QScreenCapture with initial source:

    mediaCaptureSession->setScreenCapture(screenCapture);
    mediaCaptureSession->setWindowCapture(windowCapture);
    mediaCaptureSession->setVideoOutput(videoWidget);

    // Setup UI:

    screenListView->setModel(screenListModel);
    windowListView->setModel(windowListModel);

    auto updateAction = new QAction(tr("Update Windows List"), this);
    connect(updateAction, &QAction::triggered, windowListModel, &WindowListModel::populate);
    windowListView->addAction(updateAction);
    windowListView->setContextMenuPolicy(Qt::ActionsContextMenu);

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

    connect(screenListView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &ScreenCapturePreview::onCurrentScreenSelectionChanged);
    connect(windowListView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &ScreenCapturePreview::onCurrentWindowSelectionChanged);
    connect(startStopButton, &QPushButton::clicked, this,
            &ScreenCapturePreview::onStartStopButtonClicked);
    connect(screenCapture, &QScreenCapture::errorChanged, this,
            &ScreenCapturePreview::onScreenCaptureErrorChanged, Qt::QueuedConnection);
    connect(windowCapture, &QWindowCapture::errorChanged, this,
            &ScreenCapturePreview::onWindowCaptureErrorChanged, Qt::QueuedConnection);

    updateActive(SourceType::Screen, true);
}

ScreenCapturePreview::~ScreenCapturePreview() = default;

void ScreenCapturePreview::onCurrentScreenSelectionChanged(QItemSelection selection)
{
    if (auto indexes = selection.indexes(); !indexes.empty()) {
        screenCapture->setScreen(screenListModel->screen(indexes.front()));
        updateActive(SourceType::Screen, isActive());

        windowListView->clearSelection();
    } else {
        screenCapture->setScreen(nullptr);
    }
}

void ScreenCapturePreview::onCurrentWindowSelectionChanged(QItemSelection selection)
{
    if (auto indexes = selection.indexes(); !indexes.empty()) {
        auto window = windowListModel->window(indexes.front());
        if (!window.isValid()) {
            const auto questionResult = QMessageBox::question(
                    this, tr("Invalid window"),
                    tr("The window is no longer valid. Update the list of windows?"));
            if (questionResult == QMessageBox::Yes) {
                updateActive(SourceType::Window, false);

                windowListView->clearSelection();
                windowListModel->populate();
                return;
            }
        }

        windowCapture->setWindow(window);
        updateActive(SourceType::Window, isActive());

        screenListView->clearSelection();
    } else {
        windowCapture->setWindow({});
    }
}

void ScreenCapturePreview::onWindowCaptureErrorChanged()
{
    if (windowCapture->error() == QWindowCapture::NoError)
        return;

    QMessageBox::warning(this, tr("QWindowCapture: Error occurred"), windowCapture->errorString());
}

void ScreenCapturePreview::onScreenCaptureErrorChanged()
{
    if (screenCapture->error() == QScreenCapture::NoError)
        return;

    QMessageBox::warning(this, tr("QScreenCapture: Error occurred"), screenCapture->errorString());
}

void ScreenCapturePreview::onStartStopButtonClicked()
{
    updateActive(sourceType, !isActive());
}

void ScreenCapturePreview::updateStartStopButtonText()
{
    switch (sourceType) {
    case SourceType::Window:
        startStopButton->setText(isActive() ? tr("Stop window capture")
                                            : tr("Start window capture"));
        break;
    case SourceType::Screen:
        startStopButton->setText(isActive() ? tr("Stop screen capture")
                                            : tr("Start screen capture"));
        break;
    }
}

void ScreenCapturePreview::updateActive(SourceType sourceType, bool active)
{
    this->sourceType = sourceType;

    screenCapture->setActive(active && sourceType == SourceType::Screen);
    windowCapture->setActive(active && sourceType == SourceType::Window);

    updateStartStopButtonText();
}

bool ScreenCapturePreview::isActive() const
{
    switch (sourceType) {
    case SourceType::Window:
        return windowCapture->isActive();
    case SourceType::Screen:
        return screenCapture->isActive();
    default:
        return false;
    }
}

#include "moc_screencapturepreview.cpp"

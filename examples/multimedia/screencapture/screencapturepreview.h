// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SCREENCAPTUREPREVIEW_H
#define SCREENCAPTUREPREVIEW_H

#include <QScreenCapture>
#include <QWidget>
#include <QModelIndex>

class ScreenListModel;

QT_BEGIN_NAMESPACE
class QListView;
class QMediaCaptureSession;
class QVideoWidget;
class QGridLayout;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

QT_USE_NAMESPACE

class ScreenCapturePreview : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenCapturePreview(QWidget *parent = nullptr);
    ~ScreenCapturePreview();

public slots:
    void onScreenSelectionChanged(QModelIndex index);
    void onScreenCaptureErrorOccured(QScreenCapture::Error error, const QString &errorString);
    void onStartStopButtonClicked();

private:
    ScreenListModel *screenListModel = nullptr;
    QListView *screenListView = nullptr;
    QScreenCapture *screenCapture = nullptr;
    QWindowList windows;
    QMediaCaptureSession *mediaCaptureSession = nullptr;
    QVideoWidget *videoWidget = nullptr;
    QGridLayout *gridLayout = nullptr;
    QPushButton *startStopButton = nullptr;
    QLabel *screenLabel = nullptr;
    QLabel *videoWidgetLabel = nullptr;
};

#endif // SCREENCAPTUREPREVIEW_H

// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SCREENCAPTUREPREVIEW_H
#define SCREENCAPTUREPREVIEW_H

#include <QScreenCapture>
#include <QWindowCapture>
#include <QWidget>
#include <QItemSelection>

class ScreenListModel;
class WindowListModel;

QT_BEGIN_NAMESPACE
class QListView;
class QMediaCaptureSession;
class QVideoWidget;
class QGridLayout;
class QHBoxLayout;
class QLineEdit;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

QT_USE_NAMESPACE

class ScreenCapturePreview : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenCapturePreview(QWidget *parent = nullptr);
    ~ScreenCapturePreview() override;

private slots:
    void onCurrentScreenSelectionChanged(QItemSelection index);
    void onCurrentWindowSelectionChanged(QItemSelection index);
    void onWindowCaptureErrorOccured(QWindowCapture::Error error, const QString &errorString);
    void onScreenCaptureErrorOccured(QScreenCapture::Error error, const QString &errorString);
    void onStartStopButtonClicked();

private:
    enum class SourceType { Screen, Window };

    void updateActive(SourceType sourceType, bool active);
    void updateStartStopButtonText();
    bool isActive() const;

private:
    ScreenListModel *screenListModel = nullptr;
    WindowListModel *windowListModel = nullptr;
    QListView *screenListView = nullptr;
    QListView *windowListView = nullptr;
    QScreenCapture *screenCapture = nullptr;
    QWindowCapture *windowCapture = nullptr;
    QMediaCaptureSession *mediaCaptureSession = nullptr;
    QVideoWidget *videoWidget = nullptr;
    QGridLayout *gridLayout = nullptr;
    QPushButton *startStopButton = nullptr;
    QLabel *screenLabel = nullptr;
    QLabel *windowLabel = nullptr;
    QLabel *videoWidgetLabel = nullptr;
    SourceType sourceType = SourceType::Screen;
};

#endif // SCREENCAPTUREPREVIEW_H

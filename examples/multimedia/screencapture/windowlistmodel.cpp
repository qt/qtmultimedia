// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "windowlistmodel.h"
#include <QWindowCapture>

WindowListModel::WindowListModel(QObject *parent)
    : QAbstractListModel(parent), windowList(QWindowCapture::capturableWindows())
{
}

int WindowListModel::rowCount(const QModelIndex &) const
{
    return windowList.size();
}

QVariant WindowListModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(index.isValid());
    Q_ASSERT(index.row() <= windowList.size());

    if (role == Qt::DisplayRole) {
        auto window = windowList.at(index.row());
        return window.description();
    }

    return {};
}

QCapturableWindow WindowListModel::window(const QModelIndex &index) const
{
    return windowList.at(index.row());
}

void WindowListModel::populate()
{
    beginResetModel();
    windowList = QWindowCapture::capturableWindows();
    endResetModel();
}

#include "moc_windowlistmodel.cpp"

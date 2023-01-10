// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "windowlistmodel.h"

#include <QWindow>

WindowListModel::WindowListModel(QList<QWindow *> data, QObject *parent)
    : QAbstractListModel(parent), windowList(data)
{
}

int WindowListModel::rowCount(const QModelIndex &parent) const
{
    return windowList.count();
}

QVariant WindowListModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(index.isValid());
    Q_ASSERT(index.row() <= windowList.count());

    if (role == Qt::DisplayRole) {
        auto window = windowList.at(index.row());
        return QString("%1: %2, %3")
                .arg(window->metaObject()->className())
                .arg(window->winId())
                .arg(window->objectName());
    } else {
        return QVariant();
    }
}

QWindow *WindowListModel::window(const QModelIndex &index) const
{
    return windowList.at(index.row());
}

#include "moc_windowlistmodel.cpp"

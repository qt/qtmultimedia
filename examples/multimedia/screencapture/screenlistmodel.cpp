// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "screenlistmodel.h"

#include <QScreen>

ScreenListModel::ScreenListModel(const QList<QScreen *> &data, QObject *parent) :
      QAbstractListModel(parent),
      screenList(data)
{
}

int ScreenListModel::rowCount(const QModelIndex &) const
{
    return screenList.size();
}

QVariant ScreenListModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(index.isValid());
    Q_ASSERT(index.row() <= screenList.size());

    if (role == Qt::DisplayRole) {
        auto screen = screenList.at(index.row());
        return QString("%1: %2").arg(screen->metaObject()->className(),
                                     screen->name());
    }

    return {};
}

QScreen *ScreenListModel::screen(const QModelIndex &index) const
{
    return screenList.at(index.row());
}

#include "moc_screenlistmodel.cpp"

// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "screenlistmodel.h"

#include <QGuiApplication>
#include <QScreen>

#include <QTextStream>

ScreenListModel::ScreenListModel(QObject *parent) :
      QAbstractListModel(parent)
{
    auto *app = qApp;
    connect(app, &QGuiApplication::screenAdded, this, &ScreenListModel::screensChanged);
    connect(app, &QGuiApplication::screenRemoved, this, &ScreenListModel::screensChanged);
    connect(app, &QGuiApplication::primaryScreenChanged, this, &ScreenListModel::screensChanged);
}

int ScreenListModel::rowCount(const QModelIndex &) const
{
    return QGuiApplication::screens().size();
}

QVariant ScreenListModel::data(const QModelIndex &index, int role) const
{
    const auto screenList = QGuiApplication::screens();
    Q_ASSERT(index.isValid());
    Q_ASSERT(index.row() <= screenList.size());

    if (role == Qt::DisplayRole) {
        auto *screen = screenList.at(index.row());
        QString description;
        QTextStream str(&description);
        str << '"' << screen->name() << "\" " << screen->size().width() << 'x'
            << screen->size().height() << ", " << screen->logicalDotsPerInch() << "DPI";
        return description;
    }

    return {};
}

QScreen *ScreenListModel::screen(const QModelIndex &index) const
{
    return QGuiApplication::screens().value(index.row());
}

void ScreenListModel::screensChanged()
{
    beginResetModel();
    endResetModel();
}

#include "moc_screenlistmodel.cpp"

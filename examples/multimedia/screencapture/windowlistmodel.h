// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOWLISTMODEL_H
#define WINDOWLISTMODEL_H

#include <QAbstractListModel>
#include <QCapturableWindow>

QT_USE_NAMESPACE

class WindowListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit WindowListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QCapturableWindow window(const QModelIndex &index) const;

public Q_SLOTS:
    void populate();

private:
    QList<QCapturableWindow> windowList;
};

#endif // WINDOWLISTMODEL_H

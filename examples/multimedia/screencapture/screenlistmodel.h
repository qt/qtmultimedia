// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SCREENLISTMODEL_H
#define SCREENLISTMODEL_H

#include <QAbstractListModel>

QT_BEGIN_NAMESPACE
class QScreen;
QT_END_NAMESPACE

QT_USE_NAMESPACE

class ScreenListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ScreenListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QScreen *screen(const QModelIndex &index) const;

private Q_SLOTS:
    void screensChanged();
};

#endif // SCREENLISTMODEL_H

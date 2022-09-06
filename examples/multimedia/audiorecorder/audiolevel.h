// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef AUDIOLEVEL_H
#define AUDIOLEVEL_H

#include <QWidget>

class AudioLevel : public QWidget
{
    Q_OBJECT
public:
    explicit AudioLevel(QWidget *parent = nullptr);

    // Using [0; 1.0] range
    void setLevel(qreal level);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qreal m_level = 0.0;
};

#endif // QAUDIOLEVEL_H

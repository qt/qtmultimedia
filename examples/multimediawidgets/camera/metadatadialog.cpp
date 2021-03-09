/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "metadatadialog.h"
#include "camera.h"

#include <QtWidgets>
#include <QFormLayout>
#include <QMediaMetaData>

MetaDataDialog::MetaDataDialog(QWidget *parent)
    : QDialog(parent)
{
    QFormLayout *metaDataLayout = new QFormLayout;
    for (int key = 0; key < QMediaMetaData::NumMetaData; key++) {
        QString label = QMediaMetaData::metaDataKeyToString(static_cast<QMediaMetaData::Key>(key));
        m_metaDataFields[key] = new QLineEdit;
        if (key == QMediaMetaData::ThumbnailImage) {
            QPushButton *openThumbnail = new QPushButton(tr("Open"));
            connect(openThumbnail, &QPushButton::clicked, this, &MetaDataDialog::openThumbnailImage);
            QHBoxLayout *layout = new QHBoxLayout;
            layout->addWidget(m_metaDataFields[key]);
            layout->addWidget(openThumbnail);
            metaDataLayout->addRow(label, layout);
        }
        else if (key == QMediaMetaData::CoverArtImage) {
            QPushButton *openCoverArt = new QPushButton(tr("Open"));
            connect(openCoverArt, &QPushButton::clicked, this, &MetaDataDialog::openCoverArtImage);
            QHBoxLayout *layout = new QHBoxLayout;
            layout->addWidget(m_metaDataFields[key]);
            layout->addWidget(openCoverArt);
            metaDataLayout->addRow(label, layout);
        }
        else {
            if (key == QMediaMetaData::Title)
                m_metaDataFields[key]->setText(tr("Qt Camera Example"));
            else if (key == QMediaMetaData::Author)
                m_metaDataFields[key]->setText(tr("The Qt Company"));
            else if (key == QMediaMetaData::Date)
                m_metaDataFields[key]->setText(QDateTime::currentDateTime().toString());
            else if (key == QMediaMetaData::Date)
                m_metaDataFields[key]->setText(QDate::currentDate().toString());
            metaDataLayout->addRow(label, m_metaDataFields[key]);
        }
    }

    QWidget *viewport = new QWidget;
    viewport->setLayout(metaDataLayout);
    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidget(viewport);
    QVBoxLayout *dialogLayout = new QVBoxLayout();
    this->setLayout(dialogLayout);
    this->layout()->addWidget(scrollArea);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                    | QDialogButtonBox::Cancel);
    this->layout()->addWidget(buttonBox);

    this->setWindowTitle(tr("Set Metadata"));
    this->resize(400, 300);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &MetaDataDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &MetaDataDialog::reject);
}

void MetaDataDialog::openThumbnailImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
    tr("Open Image"), QDir::currentPath(), tr("Image Files (*.png *.jpg *.bmp)"));
    if (!fileName.isEmpty())
        m_metaDataFields[QMediaMetaData::ThumbnailImage]->setText(fileName);
}

void MetaDataDialog::openCoverArtImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
    tr("Open Image"), QDir::currentPath(), tr("Image Files (*.png *.jpg *.bmp)"));
    if (!fileName.isEmpty())
        m_metaDataFields[QMediaMetaData::CoverArtImage]->setText(fileName);
}

// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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

/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef DIRECTSHOWMETADATACONTROL_H
#define DIRECTSHOWMETADATACONTROL_H

#include <qmetadatareadercontrol.h>

#include "directshowglobal.h"

#include <qnetwork.h>

#ifndef QT_NO_WMSDK
#include <wmsdk.h>
#endif

#include <QtCore/qcoreevent.h>

class DirectShowPlayerService;

QT_USE_NAMESPACE

class DirectShowMetaDataControl : public QMetaDataReaderControl
{
    Q_OBJECT
public:
    DirectShowMetaDataControl(QObject *parent = 0);
    ~DirectShowMetaDataControl();

    bool isMetaDataAvailable() const;

    QVariant metaData(QtMultimediaKit::MetaData key) const;
    QList<QtMultimediaKit::MetaData> availableMetaData() const;

    QVariant extendedMetaData(const QString &key) const;
    QStringList availableExtendedMetaData() const;

    void updateGraph(IFilterGraph2 *graph, IBaseFilter *source);

protected:
    void customEvent(QEvent *event);

private:
    enum Event
    {
        MetaDataChanged = QEvent::User
    };

    IAMMediaContent *m_content;
#ifndef QT_NO_WMSDK
    IWMHeaderInfo *m_headerInfo;
#endif
};

#endif

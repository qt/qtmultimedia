/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidmediacontainercontrol.h"

#include "qandroidcapturesession.h"

QT_BEGIN_NAMESPACE

QAndroidMediaContainerControl::QAndroidMediaContainerControl(QAndroidCaptureSession *session)
    : QMediaContainerControl()
    , m_session(session)
{
}

QStringList QAndroidMediaContainerControl::supportedContainers() const
{
    return QStringList() << QLatin1String("mp4")
                         << QLatin1String("3gp")
                         << QLatin1String("amr")
                         << QLatin1String("awb");
}

QString QAndroidMediaContainerControl::containerFormat() const
{
    return m_session->containerFormat();
}

void QAndroidMediaContainerControl::setContainerFormat(const QString &format)
{
    m_session->setContainerFormat(format);
}

QString QAndroidMediaContainerControl::containerDescription(const QString &formatMimeType) const
{
    if (formatMimeType == QLatin1String("mp4"))
        return tr("MPEG4 media file format");
    else if (formatMimeType == QLatin1String("3gp"))
        return tr("3GPP media file format");
    else if (formatMimeType == QLatin1String("amr"))
        return tr("AMR NB file format");
    else if (formatMimeType == QLatin1String("awb"))
        return tr("AMR WB file format");

    return QString();
}

QT_END_NAMESPACE

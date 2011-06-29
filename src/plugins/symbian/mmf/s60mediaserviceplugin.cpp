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

#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>

#include "s60mediaserviceplugin.h"
#if defined(TUNERLIBUSED) || defined(RADIOUTILITYLIBUSED) 
#include "s60radiotunerservice.h"
#endif
#ifdef HAS_MEDIA_PLAYER
#include "s60mediaplayerservice.h"
#endif
#ifdef AUDIOSOURCEUSED
#include "s60audiocaptureservice.h"
#endif /* AUDIOSOURCEUSED */

QStringList S60MediaServicePlugin::keys() const
{
    QStringList list;
#if defined(TUNERLIBUSED) || defined(RADIOUTILITYLIBUSED)
    list << QLatin1String(Q_MEDIASERVICE_RADIO);
#endif

#ifdef HAS_MEDIA_PLAYER  
    list << QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER);
#endif
#ifdef AUDIOSOURCEUSED
    list << QLatin1String(Q_MEDIASERVICE_AUDIOSOURCE);
#endif /* AUDIOSOURCEUSED */
    return list;
}

QMediaService* S60MediaServicePlugin::create(QString const& key)
{
#ifdef HAS_MEDIA_PLAYER
    if (key == QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER))
        return new S60MediaPlayerService;
#endif
#ifdef AUDIOSOURCEUSED
    if (key == QLatin1String(Q_MEDIASERVICE_AUDIOSOURCE))
        return new S60AudioCaptureService;
#endif /* AUDIOSOURCEUSED */
#if defined(TUNERLIBUSED) || defined(RADIOUTILITYLIBUSED) 
    if (key == QLatin1String(Q_MEDIASERVICE_RADIO)) 
        return new S60RadioTunerService;
#endif
    
    return 0;
}

void S60MediaServicePlugin::release(QMediaService *service)
{
    delete service;
}

QtMultimediaKit::SupportEstimate S60MediaServicePlugin::hasSupport(const QString &mimeType, const QStringList& codecs) const
{
    Q_UNUSED(mimeType);
    Q_UNUSED(codecs);
    return QtMultimediaKit::PreferredService;
}

QStringList S60MediaServicePlugin::supportedMimeTypes() const
{
    if (m_supportedmimetypes.isEmpty()) {
        TInt err;
        S60FormatSupported* formats = new (ELeave) S60FormatSupported();
        if (formats) {
            TRAP(err, m_supportedmimetypes = formats->supportedPlayMimeTypesL());
            delete formats;
        }
    }
    return m_supportedmimetypes;
}

Q_EXPORT_PLUGIN2(qtmultimediakit_mmfengine, S60MediaServicePlugin);

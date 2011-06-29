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


#include "s60formatsupported.h"



S60FormatSupported::S60FormatSupported()
{}

S60FormatSupported::~S60FormatSupported()
{
    if (m_controllerparam) {
        delete m_controllerparam;
        m_controllerparam = NULL;
    }
}

QStringList S60FormatSupported::supportedPlayMimeTypesL()
{

    RArray<TUid> mediaIds; //search for both audio and video

    RMMFControllerImplInfoArray iControllers;

    m_controllerparam = CMMFControllerPluginSelectionParameters::NewL();

    m_playformatparam = CMMFFormatSelectionParameters::NewL();

    mediaIds.Append(KUidMediaTypeAudio);

    mediaIds.Append(KUidMediaTypeVideo);

    m_controllerparam->SetMediaIdsL(mediaIds, CMMFPluginSelectionParameters::EAllowOtherMediaIds);

    m_controllerparam->SetRequiredPlayFormatSupportL(*m_playformatparam);

    m_controllerparam->ListImplementationsL(iControllers);

    CDesC8ArrayFlat* controllerArray = new (ELeave) CDesC8ArrayFlat(1);

    for (TInt i = 0; i < iControllers.Count(); i++) {
        for (TInt j = 0; j < (iControllers[i]->PlayFormats()).Count(); j++) {
            const CDesC8Array& iarr = (iControllers[i]->PlayFormats()[j]->SupportedMimeTypes());

            TInt count = iarr.Count();

            for (TInt k = 0; k < count; k++) {
                TPtrC8 ptr = iarr.MdcaPoint(k);

                HBufC8* n = HBufC8::NewL(ptr.Length());

                TPtr8 ptr1 = n->Des();

                ptr1.Copy((TUint8*) ptr.Ptr(), ptr.Length());

                controllerArray->AppendL(ptr1);
            }
        }
    }

// converting CDesC8Array to QStringList
    for (TInt x = 0; x < controllerArray->Count(); x++) {
        m_supportedplaymime.append(QString::fromUtf8(
                (const char*) (controllerArray->MdcaPoint(x).Ptr()),
                controllerArray->MdcaPoint(x).Length()));
    }

    // populating the list with only audio and controller mime types
    QStringList tempaudio = m_supportedplaymime.filter(QString("audio"));
    QStringList tempvideo = m_supportedplaymime.filter(QString("video"));

    m_supportedplaymime.clear();

    m_supportedplaymime = tempaudio + tempvideo;

    mediaIds.Close();
    delete controllerArray;
    iControllers.ResetAndDestroy();

    return m_supportedplaymime;
}

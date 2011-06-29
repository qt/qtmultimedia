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

#ifndef XARECORDSESSIONIMPL_H
#define XARECORDSESSIONIMPL_H

#include <OpenMAXAL.h>
#include <badesca.h>


class XARecordObserver;
class XARecordSessionImpl
{
public:
    XARecordSessionImpl(XARecordObserver &parent);
    ~XARecordSessionImpl();
    TInt32 postConstruct();

    /* For QMediaRecorderControl begin */
    TInt32 setURI(const TDesC &aURI);
    TInt32 record();
    TInt32 pause();
    TInt32 stop();
    TInt32 duration(TInt64 &aDur);
    /* For QMediaRecorderControl end */

    void cbMediaRecorder(XAObjectItf caller,
                               const void *pContext,
                               XAuint32 event,
                               XAresult result,
                               XAuint32 param,
                               void *pInterface);
    void cbRecordItf(XARecordItf caller,
                           void *pContext,
                           XAuint32 event);

    /* For QAudioEndpointSelector begin */
    void getAudioInputDeviceNames(RArray<TPtrC> &aArray);
    TInt32 defaultAudioInputDevice(TPtrC &endPoint);
    TInt32 activeAudioInputDevice(TPtrC &endPoint);
    TBool setAudioInputDevice(const TDesC &aDevice);
    void cbAvailableAudioInputsChanged(XAAudioIODeviceCapabilitiesItf caller,
                                        void *pContext,
                                        XAuint32 deviceID,
                                        XAint32 numInputs,
                                        XAboolean isNew);
    /* For QAudioEndpointSelector end */

    /* For QAudioEncoderControl begin */
    const RArray<TPtrC>& getAudioEncoderNames();
    TInt32 getSampleRates(const TDesC &aEncoder,
                          RArray<TInt32> &aSampleRates,
                          TBool &aIsContinuous);
    TInt32 getBitrates(const TDesC &aEncoder,
                          RArray<TUint32> &aBitrates,
                          TBool& aContinuous);
    /* For QAudioEncoderControl end */

    /* For QMediaContainerControl begin */
    const RArray<TPtrC>& getContainerNames();
    const RArray<TPtrC>& getContainerDescs();
    /* For QMediaContainerControl end */

    void resetEncoderAttributes();
    void setContainerType(const TDesC &aURI);
    TBool setCodec(const TDesC &aURI);
    TUint32 getBitRate();
    void setBitRate(TUint32 aBitRate);
    TUint32 getChannels();
    void setChannels(TUint32 aChannels);
    void setOptimalChannelCount();
    TUint32 getSampleRate();
    void setSampleRate(TUint32 aSampleRate);
    void setOptimalSampleRate();
    TInt32 setCBRMode();
    TInt32 setVBRMode();
    void setVeryLowQuality();
    void setLowQuality();
    void setNormalQuality();
    void setHighQuality();
    void setVeryHighQuality();

private:
    TInt32 createMediaRecorderObject();
    TInt32 mapError(XAresult xa_err,
                  TBool debPrn);
    TInt32 initContainersList();
    TInt32 initAudioEncodersList();
    TInt32 initAudioInputDevicesList();
    TInt32 setEncoderSettingsToMediaRecorder();
    TInt32 getBitratesByAudioCodecID(XAuint32 encoderId,
                          RArray<TUint32> &aBitrates,
                          XAboolean& aContinuous);
    TInt32 getSampleRatesByAudioCodecID(XAuint32 encoderId,
                          RArray<TInt32> &aSampleRates);


private:
    XARecordObserver &m_Parent;
    XAObjectItf m_EOEngine;
    XAObjectItf m_MORecorder;
    XARecordItf m_RecordItf;
    XAAudioEncoderItf m_AudioEncItf;
    /* Audio Source */
    XADataSource m_DataSource;
    XADataLocator_IODevice m_LocatorMic;
    XADataFormat_MIME m_Mime;
    XADataLocator_URI m_Uri;
    /*Audio Sink*/
    XADataSink m_DataSink;
    HBufC8 *m_WAVMime;

    /* Set by client*/
    HBufC8 *m_URIName;
    XAuint32 m_AudioEncoderId;
    XAuint32 m_InputDeviceId;
    XAuint32 m_ContainerType;
    XAuint32 m_BitRate;
    XAuint32 m_RateControl;
    XAuint32 m_ProfileSetting;
    XAuint32 m_ChannelsOut;
    XAuint32 m_SampleRate;

    /* For QAudioEndpointSelector begin */
    XAAudioIODeviceCapabilitiesItf m_AudioIODevCapsItf;
    RArray<TUint32> m_InputDeviceIDs;
    CDesC16ArrayFlat *m_AudioInputDeviceNames;
    RArray<TUint32> m_DefaultInputDeviceIDs;
    CDesC16ArrayFlat *m_DefaultAudioInputDeviceNames;
    /* For QAudioEndpointSelector end */

    /* For QAudioEncoderControl begin */
    XAAudioEncoderCapabilitiesItf m_AudioEncCapsItf;
    RArray<XAuint32> m_EncoderIds;
    RArray<TPtrC> m_EncoderNames;
    RArray<TPtrC> m_ContainerNames;
    RArray<TPtrC> m_ContainerDescs;
    /* For QAudioEncoderControl begin */
};

#endif /* XARECORDSESSIONIMPL_H */

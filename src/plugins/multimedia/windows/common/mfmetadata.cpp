// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qmediametadata.h>
#include <qdatetime.h>
#include <qtimezone.h>
#include <qimage.h>
#include <quuid.h>

#include <guiddef.h>
#include <cguid.h>
#include <mfapi.h>
#include <mfidl.h>
#include <propvarutil.h>
#include <propkey.h>

#include "private/qwindowsmultimediautils_p.h"
#include "mfmetadata_p.h"

//#define DEBUG_MEDIAFOUNDATION

static const PROPERTYKEY PROP_KEY_NULL = {GUID_NULL, 0};

static QVariant convertValue(const PROPVARIANT& var)
{
    QVariant value;
    switch (var.vt) {
    case VT_LPWSTR:
        value = QString::fromUtf16(reinterpret_cast<const char16_t *>(var.pwszVal));
        break;
    case VT_UI4:
        value = uint(var.ulVal);
        break;
    case VT_UI8:
        value = qulonglong(var.uhVal.QuadPart);
        break;
    case VT_BOOL:
        value = bool(var.boolVal);
        break;
    case VT_FILETIME:
        SYSTEMTIME t;
        if (!FileTimeToSystemTime(&var.filetime, &t))
            break;

        value = QDateTime(QDate(t.wYear, t.wMonth, t.wDay),
                          QTime(t.wHour, t.wMinute, t.wSecond, t.wMilliseconds),
                          QTimeZone(QTimeZone::UTC));
        break;
    case VT_STREAM:
    {
        STATSTG stat;
        if (FAILED(var.pStream->Stat(&stat, STATFLAG_NONAME)))
            break;
        void *data = malloc(stat.cbSize.QuadPart);
        ULONG read = 0;
        if (FAILED(var.pStream->Read(data, stat.cbSize.QuadPart, &read))) {
            free(data);
            break;
        }
        value = QImage::fromData((const uchar*)data, read);
        free(data);
    }
        break;
    case VT_VECTOR | VT_LPWSTR:
        QStringList vList;
        for (ULONG i = 0; i < var.calpwstr.cElems; ++i)
            vList.append(QString::fromUtf16(reinterpret_cast<const char16_t *>(var.calpwstr.pElems[i])));
        value = vList;
        break;
    }
    return value;
}

static QVariant metaDataValue(IPropertyStore *content, const PROPERTYKEY &key)
{
    QVariant value;

    PROPVARIANT var;
    PropVariantInit(&var);
    HRESULT hr = S_FALSE;
    if (content)
        hr = content->GetValue(key, &var);

    if (SUCCEEDED(hr)) {
        value = convertValue(var);

        // some metadata needs to be reformatted
        if (value.isValid() && content) {
            if (key == PKEY_Media_ClassPrimaryID /*QMediaMetaData::MediaType*/) {
                QString v = value.toString();
                if (v == QLatin1String("{D1607DBC-E323-4BE2-86A1-48A42A28441E}"))
                    value = QStringLiteral("Music");
                else if (v == QLatin1String("{DB9830BD-3AB3-4FAB-8A37-1A995F7FF74B}"))
                    value = QStringLiteral("Video");
                else if (v == QLatin1String("{01CD0F29-DA4E-4157-897B-6275D50C4F11}"))
                    value = QStringLiteral("Audio");
                else if (v == QLatin1String("{FCF24A76-9A57-4036-990D-E35DD8B244E1}"))
                    value = QStringLiteral("Other");
            } else if (key == PKEY_Media_Duration) {
                // duration is provided in 100-nanosecond units, convert to milliseconds
                value = (value.toLongLong() + 10000) / 10000;
            } else if (key == PKEY_Video_Compression) {
                value = int(QWindowsMultimediaUtils::codecForVideoFormat(value.toUuid()));
            } else if (key == PKEY_Audio_Format) {
                value = int(QWindowsMultimediaUtils::codecForAudioFormat(value.toUuid()));
            } else if (key == PKEY_Video_FrameHeight /*Resolution*/) {
                QSize res;
                res.setHeight(value.toUInt());
                if (content && SUCCEEDED(content->GetValue(PKEY_Video_FrameWidth, &var)))
                    res.setWidth(convertValue(var).toUInt());
                value = res;
            } else if (key == PKEY_Video_Orientation) {
                uint orientation = 0;
                if (content && SUCCEEDED(content->GetValue(PKEY_Video_Orientation, &var)))
                    orientation = convertValue(var).toUInt();
                value = orientation;
            } else if (key == PKEY_Video_FrameRate) {
                value = value.toReal() / 1000.f;
            }
        }
    }

    PropVariantClear(&var);
    return value;
}

QMediaMetaData MFMetaData::fromNative(IMFMediaSource* mediaSource)
{
    QMediaMetaData metaData;

    IPropertyStore  *content = nullptr;
    if (!SUCCEEDED(MFGetService(mediaSource, MF_PROPERTY_HANDLER_SERVICE, IID_PPV_ARGS(&content))))
        return metaData;

    Q_ASSERT(content);
    DWORD cProps;
    if (SUCCEEDED(content->GetCount(&cProps))) {
        for (DWORD i = 0; i < cProps; i++)
        {
            PROPERTYKEY key;
            if (FAILED(content->GetAt(i, &key)))
                continue;
            QMediaMetaData::Key mediaKey;
            if (key == PKEY_Author) {
                mediaKey = QMediaMetaData::Author;
            } else if (key == PKEY_Title) {
                mediaKey = QMediaMetaData::Title;
//            } else if (key == PKEY_Media_SubTitle) {
//                mediaKey = QMediaMetaData::SubTitle;
//            } else if (key == PKEY_ParentalRating) {
//                mediaKey = QMediaMetaData::ParentalRating;
            } else if (key == PKEY_Media_EncodingSettings) {
                mediaKey = QMediaMetaData::Description;
            } else if (key == PKEY_Copyright) {
                mediaKey = QMediaMetaData::Copyright;
            } else if (key == PKEY_Comment) {
                mediaKey = QMediaMetaData::Comment;
            } else if (key == PKEY_Media_ProviderStyle) {
                mediaKey = QMediaMetaData::Genre;
            } else if (key == PKEY_Media_DateEncoded) {
                mediaKey = QMediaMetaData::Date;
//            } else if (key == PKEY_Rating) {
//                mediaKey = QMediaMetaData::UserRating;
//            } else if (key == PKEY_Keywords) {
//                mediaKey = QMediaMetaData::Keywords;
            } else if (key == PKEY_Language) {
                mediaKey = QMediaMetaData::Language;
            } else if (key == PKEY_Media_Publisher) {
                mediaKey = QMediaMetaData::Publisher;
            } else if (key == PKEY_Media_ClassPrimaryID) {
                mediaKey = QMediaMetaData::MediaType;
            } else if (key == PKEY_Media_Duration) {
                mediaKey = QMediaMetaData::Duration;
            } else if (key == PKEY_Audio_EncodingBitrate) {
                mediaKey = QMediaMetaData::AudioBitRate;
            } else if (key == PKEY_Audio_Format) {
                mediaKey = QMediaMetaData::AudioCodec;
//            } else if (key == PKEY_Media_AverageLevel) {
//                mediaKey = QMediaMetaData::AverageLevel;
//            } else if (key == PKEY_Audio_ChannelCount) {
//                mediaKey = QMediaMetaData::ChannelCount;
//            } else if (key == PKEY_Audio_PeakValue) {
//                mediaKey = QMediaMetaData::PeakValue;
//            } else if (key == PKEY_Audio_SampleRate) {
//                mediaKey = QMediaMetaData::SampleRate;
            } else if (key == PKEY_Music_AlbumTitle) {
                mediaKey = QMediaMetaData::AlbumTitle;
            } else if (key == PKEY_Music_AlbumArtist) {
                mediaKey = QMediaMetaData::AlbumArtist;
            } else if (key == PKEY_Music_Artist) {
                mediaKey = QMediaMetaData::ContributingArtist;
            } else if (key == PKEY_Music_Composer) {
                mediaKey = QMediaMetaData::Composer;
//            } else if (key == PKEY_Music_Conductor) {
//                mediaKey = QMediaMetaData::Conductor;
//            } else if (key == PKEY_Music_Lyrics) {
//                mediaKey = QMediaMetaData::Lyrics;
//            } else if (key == PKEY_Music_Mood) {
//                mediaKey = QMediaMetaData::Mood;
            } else if (key == PKEY_Music_TrackNumber) {
                mediaKey = QMediaMetaData::TrackNumber;
            } else if (key == PKEY_Music_Genre) {
                mediaKey = QMediaMetaData::Genre;
            } else if (key == PKEY_ThumbnailStream) {
                mediaKey = QMediaMetaData::ThumbnailImage;
            } else if (key == PKEY_Video_FrameHeight) {
                mediaKey = QMediaMetaData::Resolution;
            } else if (key == PKEY_Video_Orientation) {
                mediaKey = QMediaMetaData::Orientation;
            } else if (key == PKEY_Video_FrameRate) {
                mediaKey = QMediaMetaData::VideoFrameRate;
            } else if (key == PKEY_Video_EncodingBitrate) {
                mediaKey = QMediaMetaData::VideoBitRate;
            } else if (key == PKEY_Video_Compression) {
                mediaKey = QMediaMetaData::VideoCodec;
//            } else if (key == PKEY_Video_Director) {
//                mediaKey = QMediaMetaData::Director;
//            } else if (key == PKEY_Media_Writer) {
//                mediaKey = QMediaMetaData::Writer;
            } else {
                continue;
            }
            metaData.insert(mediaKey, metaDataValue(content, key));
        }
    }

    content->Release();

    return metaData;
}

static REFPROPERTYKEY propertyKeyForMetaDataKey(QMediaMetaData::Key key)
{
    switch (key) {
    case QMediaMetaData::Key::Title:
        return PKEY_Title;
    case QMediaMetaData::Key::Author:
        return PKEY_Author;
    case QMediaMetaData::Key::Comment:
        return PKEY_Comment;
    case QMediaMetaData::Key::Genre:
        return PKEY_Music_Genre;
    case QMediaMetaData::Key::Copyright:
        return PKEY_Copyright;
    case QMediaMetaData::Key::Publisher:
        return PKEY_Media_Publisher;
    case QMediaMetaData::Key::Url:
        return PKEY_Media_AuthorUrl;
    case QMediaMetaData::Key::AlbumTitle:
        return PKEY_Music_AlbumTitle;
    case QMediaMetaData::Key::AlbumArtist:
        return PKEY_Music_AlbumArtist;
    case QMediaMetaData::Key::TrackNumber:
        return PKEY_Music_TrackNumber;
    case QMediaMetaData::Key::Date:
        return PKEY_Media_DateEncoded;
    case QMediaMetaData::Key::Composer:
        return PKEY_Music_Composer;
    case QMediaMetaData::Key::Duration:
        return PKEY_Media_Duration;
    case QMediaMetaData::Key::Language:
        return PKEY_Language;
    case QMediaMetaData::Key::Description:
        return PKEY_Media_EncodingSettings;
    case QMediaMetaData::Key::AudioBitRate:
        return PKEY_Audio_EncodingBitrate;
    case QMediaMetaData::Key::ContributingArtist:
        return PKEY_Music_Artist;
    case QMediaMetaData::Key::ThumbnailImage:
        return PKEY_ThumbnailStream;
    case QMediaMetaData::Key::Orientation:
        return PKEY_Video_Orientation;
    case QMediaMetaData::Key::VideoFrameRate:
        return PKEY_Video_FrameRate;
    case QMediaMetaData::Key::VideoBitRate:
        return PKEY_Video_EncodingBitrate;
    case QMediaMetaData::MediaType:
        return PKEY_Media_ClassPrimaryID;
    default:
        return PROP_KEY_NULL;
    }
}

static void setStringProperty(IPropertyStore *content, REFPROPERTYKEY key, const QString &value)
{
    PROPVARIANT propValue = {};
    if (SUCCEEDED(InitPropVariantFromString(reinterpret_cast<LPCWSTR>(value.utf16()), &propValue))) {
        if (SUCCEEDED(PSCoerceToCanonicalValue(key, &propValue)))
            content->SetValue(key, propValue);
        PropVariantClear(&propValue);
    }
}

static void setUInt32Property(IPropertyStore *content, REFPROPERTYKEY key, quint32 value)
{
    PROPVARIANT propValue = {};
    if (SUCCEEDED(InitPropVariantFromUInt32(ULONG(value), &propValue))) {
        if (SUCCEEDED(PSCoerceToCanonicalValue(key, &propValue)))
            content->SetValue(key, propValue);
        PropVariantClear(&propValue);
    }
}

static void setUInt64Property(IPropertyStore *content, REFPROPERTYKEY key, quint64 value)
{
    PROPVARIANT propValue = {};
    if (SUCCEEDED(InitPropVariantFromUInt64(ULONGLONG(value), &propValue))) {
        if (SUCCEEDED(PSCoerceToCanonicalValue(key, &propValue)))
            content->SetValue(key, propValue);
        PropVariantClear(&propValue);
    }
}

static void setFileTimeProperty(IPropertyStore *content, REFPROPERTYKEY key, const FILETIME *ft)
{
    PROPVARIANT propValue = {};
    if (SUCCEEDED(InitPropVariantFromFileTime(ft, &propValue))) {
        if (SUCCEEDED(PSCoerceToCanonicalValue(key, &propValue)))
            content->SetValue(key, propValue);
        PropVariantClear(&propValue);
    }
}

void MFMetaData::toNative(const QMediaMetaData &metaData, IPropertyStore *content)
{
    if (content) {

        for (const auto &key : metaData.keys()) {

            QVariant value = metaData.value(key);

            if (key == QMediaMetaData::Key::MediaType) {

                QString strValue = metaData.stringValue(key);
                QString v;

                // Sets property to one of the MediaClassPrimaryID values defined by Microsoft:
                // https://docs.microsoft.com/en-us/windows/win32/wmformat/wm-mediaprimaryid
                if (strValue == QLatin1String("Music"))
                    v = QLatin1String("{D1607DBC-E323-4BE2-86A1-48A42A28441E}");
                else if (strValue == QLatin1String("Video"))
                    v = QLatin1String("{DB9830BD-3AB3-4FAB-8A37-1A995F7FF74B}");
                else if (strValue == QLatin1String("Audio"))
                    v = QLatin1String("{01CD0F29-DA4E-4157-897B-6275D50C4F11}");
                else
                    v = QLatin1String("{FCF24A76-9A57-4036-990D-E35DD8B244E1}");

                setStringProperty(content, PKEY_Media_ClassPrimaryID, v);

            } else if (key == QMediaMetaData::Key::Duration) {

                setUInt64Property(content, PKEY_Media_Duration, value.toULongLong() * 10000);

            } else if (key == QMediaMetaData::Key::Resolution) {

                QSize res = value.toSize();
                setUInt32Property(content, PKEY_Video_FrameWidth, quint32(res.width()));
                setUInt32Property(content, PKEY_Video_FrameHeight, quint32(res.height()));

            } else if (key == QMediaMetaData::Key::Orientation) {

                setUInt32Property(content, PKEY_Video_Orientation, value.toUInt());

            } else if (key == QMediaMetaData::Key::VideoFrameRate) {

                qreal fps = value.toReal();
                setUInt32Property(content, PKEY_Video_FrameRate, quint32(fps * 1000));

            } else if (key == QMediaMetaData::Key::TrackNumber) {

                setUInt32Property(content, PKEY_Music_TrackNumber, value.toUInt());

            } else if (key == QMediaMetaData::Key::AudioBitRate) {

                setUInt32Property(content, PKEY_Audio_EncodingBitrate, value.toUInt());

            } else if (key == QMediaMetaData::Key::VideoBitRate) {

                setUInt32Property(content, PKEY_Video_EncodingBitrate, value.toUInt());

            } else if (key == QMediaMetaData::Key::Date) {

                // Convert QDateTime to FILETIME by converting to 100-nsecs since
                // 01/01/1970 UTC and adding the difference from 1601 to 1970.
                ULARGE_INTEGER t = {};
                t.QuadPart = ULONGLONG(value.toDateTime().toUTC().toMSecsSinceEpoch() * 10000
                                       + 116444736000000000LL);

                FILETIME ft = {};
                ft.dwHighDateTime = t.HighPart;
                ft.dwLowDateTime = t.LowPart;

                setFileTimeProperty(content, PKEY_Media_DateEncoded, &ft);

            } else {

                // By default use as string and let PSCoerceToCanonicalValue()
                // do validation and type conversion.
                REFPROPERTYKEY propKey = propertyKeyForMetaDataKey(key);

                if (propKey != PROP_KEY_NULL) {
                    QString strValue = metaData.stringValue(key);
                    if (!strValue.isEmpty())
                        setStringProperty(content, propKey, strValue);
                }
            }
        }
    }
}


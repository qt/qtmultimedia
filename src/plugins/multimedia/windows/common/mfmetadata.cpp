// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "mfmetadata_p.h"

#include <QtMultimedia/qmediametadata.h>
#include <QtMultimedia/private/qmediametadata_p.h>
#include <QtMultimedia/private/qwindows_scopedpropvariant_p.h>
#include <QtMultimedia/private/qwindowsmultimediautils_p.h>
#include <QtGui/qimage.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qtimezone.h>
#include <QtCore/quuid.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/private/qcomptr_p.h>
#include <QtCore/private/qflatmap_p.h>

#include <cguid.h>
#include <guiddef.h>
#include <mfapi.h>
#include <mfidl.h>
#include <propkey.h>
#include <propvarutil.h>
#include <wmsdkidl.h>

//#define DEBUG_MEDIAFOUNDATION

using namespace Qt::StringLiterals;

static const PROPERTYKEY PROP_KEY_NULL = {GUID_NULL, 0};

static QVariant convertValue(const PROPVARIANT& var)
{
    QVariant value;
    switch (var.vt) {
    case VT_LPWSTR:
        value = QString::fromUtf16(reinterpret_cast<const char16_t *>(var.pwszVal));
        break;
    case VT_I4:
        value = int(var.lVal);
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
    if (!content)
        return {};

    QtMultimediaPrivate::ScopedPropVariant pv;
    if (FAILED(content->GetValue(key, pv.get())))
        return {};

    QVariant value = convertValue(pv.var);
    if (!value.isValid())
        return value;

    // some metadata needs to be reformatted
    if (key == PKEY_Media_ClassPrimaryID /*QMediaMetaData::MediaType*/) {
        QString v = value.toString();
        if (v == u"{D1607DBC-E323-4BE2-86A1-48A42A28441E}")
            value = u"Music"_s;
        else if (v == u"{DB9830BD-3AB3-4FAB-8A37-1A995F7FF74B}")
            value = u"Video"_s;
        else if (v == u"{01CD0F29-DA4E-4157-897B-6275D50C4F11}")
            value = u"Audio"_s;
        else if (v == u"{FCF24A76-9A57-4036-990D-E35DD8B244E1}")
            value = u"Other"_s;
    } else if (key == PKEY_Media_Duration) {
        // duration is provided in 100-nanosecond units, convert to milliseconds
        value = (value.toLongLong() + 10000) / 10000;
    } else if (key == PKEY_Video_Compression) {
        value = int(QWMF::codecForVideoFormat(value.toUuid()));
    } else if (key == PKEY_Audio_Format) {
        value = int(QWMF::codecForAudioFormat(value.toUuid()));
    } else if (key == PKEY_Video_FrameHeight /*Resolution*/) {
        QSize res;
        res.setHeight(value.toUInt());
        if (SUCCEEDED(content->GetValue(PKEY_Video_FrameWidth, pv.get())))
            res.setWidth(convertValue(pv.var).toUInt());
        value = res;
    } else if (key == PKEY_Video_Orientation) {
        uint orientation = 0;
        if (SUCCEEDED(content->GetValue(PKEY_Video_Orientation, pv.get())))
            orientation = convertValue(pv.var).toUInt();
        value = orientation;
    } else if (key == PKEY_Video_FrameRate) {
        value = value.toReal() / 1000.f;
    }

    return value;
}

QMediaMetaData MFMetaData::fromNative(IMFMediaSource* mediaSource)
{
    QMediaMetaData metaData;

    // Shell property handler first — provides the richest metadata
    // (thumbnails, duration, codecs, resolution, bitrates, etc.)
    // but only works for file:// sources.
    ComPtr<IPropertyStore> content;
    if (SUCCEEDED(MFGetService(mediaSource, MF_PROPERTY_HANDLER_SERVICE, IID_PPV_ARGS(&content))))
        metaData = fromNative(content.Get());

    // IMFMetadataProvider fallback — works for all source types
    // including byte streams (qrc://, QIODevice). Fills in any keys
    // not already provided by IPropertyStore.
    ComPtr<IMFMetadataProvider> provider;
    if (SUCCEEDED(MFGetService(mediaSource, MF_METADATA_PROVIDER_SERVICE, IID_PPV_ARGS(&provider)))) {
        ComPtr<IMFPresentationDescriptor> pd;
        if (SUCCEEDED(mediaSource->CreatePresentationDescriptor(&pd))) {
            ComPtr<IMFMetadata> metadata;
            if (SUCCEEDED(provider->GetMFMetadata(pd.Get(), 0, 0, &metadata))) {
                const QMediaMetaData mfData = fromNative(metadata.Get());
                for (const auto &[key, value] : mfData.asKeyValueRange()) {
                    if (!metaData.value(key).isValid())
                        metaData.insert(key, value);
                }
            }
        }
    }

    return metaData;
}

// MinGW's mfidl.h declares ASF_FLAT_PICTURE without the #pragma pack(1) that the
// Windows SDK applies, making sizeof() 8 there instead of 5. Use our own packed
// definition on both toolchains, so there is a single code path guarded by the
// static_assert below.
#pragma pack(push, 1)
struct QMM_ASF_FLAT_PICTURE
{
    BYTE bPictureType;
    DWORD dwDataLen;
};
#pragma pack(pop)
static_assert(sizeof(QMM_ASF_FLAT_PICTURE) == 5);
static_assert(alignof(QMM_ASF_FLAT_PICTURE) == 1);

static QImage imageFromAsfFlatPicture(const BLOB &blob)
{
    if (blob.cbSize <= sizeof(QMM_ASF_FLAT_PICTURE))
        return {};

    const auto *pic = reinterpret_cast<const QMM_ASF_FLAT_PICTURE *>(blob.pBlobData);
    const BYTE *p = blob.pBlobData + sizeof(QMM_ASF_FLAT_PICTURE);
    const BYTE *end = blob.pBlobData + blob.cbSize;

    // Skip MIME type (null-terminated UTF-16)
    while (p + 1 < end && (p[0] || p[1]))
        p += 2;
    p += 2;
    if (p > end)
        return {};

    // Skip description (null-terminated UTF-16)
    while (p + 1 < end && (p[0] || p[1]))
        p += 2;
    p += 2;
    if (p > end || pic->dwDataLen > static_cast<DWORD>(end - p))
        return {};

    QImage img;
    img.loadFromData(p, pic->dwDataLen);
    return img;
}

QMediaMetaData MFMetaData::fromNative(IMFMetadata *metadata)
{
    if (!metadata)
        return {};

    QtMultimediaPrivate::ScopedPropVariant names;
    if (FAILED(metadata->GetAllPropertyNames(names.get())))
        return {};

    QMediaMetaData metaData;

    // Property name strings match the Windows SDK g_wszWM* constants from
    // wmsdkidl.h but are hardcoded here as they are missing in older MinGW
    // variants of the Windows SDK.
    static const QVarLengthFlatMap<QStringView, QMediaMetaData::Key, 12> nameToKey({
        { u"Title", QMediaMetaData::Title },
        { u"Author", QMediaMetaData::ContributingArtist },
        { u"WM/AlbumTitle", QMediaMetaData::AlbumTitle },
        { u"WM/AlbumArtist", QMediaMetaData::AlbumArtist },
        { u"WM/Composer", QMediaMetaData::Composer },
        { u"WM/Genre", QMediaMetaData::Genre },
        { u"WM/TrackNumber", QMediaMetaData::TrackNumber },
        { u"Description", QMediaMetaData::Description },
        { u"Copyright", QMediaMetaData::Copyright },
        { u"WM/Publisher", QMediaMetaData::Publisher },
        { u"WM/Language", QMediaMetaData::Language },
        { u"WM/AuthorURL", QMediaMetaData::Url },
    });

    if (names->vt == (VT_VECTOR | VT_LPWSTR)) {
        for (ULONG i = 0; i < names->calpwstr.cElems; ++i) {
            const QStringView name(names->calpwstr.pElems[i]);

            // WM/Picture blob: QMM_ASF_FLAT_PICTURE header followed by
            // MIME type string, description string, and image data.
            if (name == u"WM/Picture") {
                QtMultimediaPrivate::ScopedPropVariant value;
                if (SUCCEEDED(metadata->GetProperty(names->calpwstr.pElems[i], value.get()))
                    && value->vt == VT_BLOB) {
                    QImage img = imageFromAsfFlatPicture(value->blob);
                    if (!img.isNull())
                        metaData.insert(QMediaMetaData::CoverArtImage, img);
                }
                continue;
            }

            auto it = nameToKey.find(name);
            if (it == nameToKey.end())
                continue;

            QtMultimediaPrivate::ScopedPropVariant value;
            if (SUCCEEDED(metadata->GetProperty(names->calpwstr.pElems[i], value.get()))) {
                QVariant v = convertValue(value.var);
                if (v.isValid())
                    metaData.insert(it.value(), v);
            }
        }
    }

    return metaData;
}

QMediaMetaData MFMetaData::fromNative(IPropertyStore *content)
{
    QMediaMetaData metaData;

    if (!content)
        return metaData;

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
                QVariant val = metaDataValue(content, key);
                if (val.canConvert<QImage>())
                    QtMultimediaPrivate::setCoverArtImage(metaData, val.value<QImage>());
                continue;
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
#if QT_DEPRECATED_SINCE(6, 12)
    case QtMultimediaPrivate::deprecatedThumbnailImage:
#endif
    case QMediaMetaData::Key::CoverArtImage:
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
    QtMultimediaPrivate::ScopedPropVariant propValue;
    if (SUCCEEDED(InitPropVariantFromString(reinterpret_cast<LPCWSTR>(value.utf16()), propValue.get()))) {
        if (SUCCEEDED(PSCoerceToCanonicalValue(key, propValue.get())))
            content->SetValue(key, propValue.var);
    }
}

static void setUInt32Property(IPropertyStore *content, REFPROPERTYKEY key, quint32 value)
{
    QtMultimediaPrivate::ScopedPropVariant propValue;
    if (SUCCEEDED(InitPropVariantFromUInt32(ULONG(value), propValue.get()))) {
        if (SUCCEEDED(PSCoerceToCanonicalValue(key, propValue.get())))
            content->SetValue(key, propValue.var);
    }
}

static void setUInt64Property(IPropertyStore *content, REFPROPERTYKEY key, quint64 value)
{
    QtMultimediaPrivate::ScopedPropVariant propValue;
    if (SUCCEEDED(InitPropVariantFromUInt64(ULONGLONG(value), propValue.get()))) {
        if (SUCCEEDED(PSCoerceToCanonicalValue(key, propValue.get())))
            content->SetValue(key, propValue.var);
    }
}

static void setFileTimeProperty(IPropertyStore *content, REFPROPERTYKEY key, const FILETIME *ft)
{
    QtMultimediaPrivate::ScopedPropVariant propValue;
    if (SUCCEEDED(InitPropVariantFromFileTime(ft, propValue.get()))) {
        if (SUCCEEDED(PSCoerceToCanonicalValue(key, propValue.get())))
            content->SetValue(key, propValue.var);
    }
}

void MFMetaData::toNative(const QMediaMetaData &metaData, IPropertyStore *content)
{
    Q_ASSERT(content);

    for (const auto &key : metaData.keys()) {

        QVariant value = metaData.value(key);

        if (key == QMediaMetaData::Key::MediaType) {

            QString strValue = metaData.stringValue(key);
            QString v;

            // Sets property to one of the MediaClassPrimaryID values defined by Microsoft:
            // https://docs.microsoft.com/en-us/windows/win32/wmformat/wm-mediaprimaryid
            if (strValue == u"Music")
                v = u"{D1607DBC-E323-4BE2-86A1-48A42A28441E}"_s;
            else if (strValue == u"Video")
                v = u"{DB9830BD-3AB3-4FAB-8A37-1A995F7FF74B}"_s;
            else if (strValue == u"Audio")
                v = u"{01CD0F29-DA4E-4157-897B-6275D50C4F11}"_s;
            else
                v = u"{FCF24A76-9A57-4036-990D-E35DD8B244E1}"_s;

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

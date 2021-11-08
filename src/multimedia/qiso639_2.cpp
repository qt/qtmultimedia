/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qiso639_2_p.h"

QT_BEGIN_NAMESPACE

/*
    This is a list of iso639-2/B language codes, as those are being used for
    language tagging in multimedia files. QLocale only supports iso639-1, so we
    can't use the conversion methods there.

    Also add the iso639-2/T codes, so we can recognize them, but never use them
    when setting a language code, as the /B variants are preferred.
*/

static const QLocale::Language lastLanguage = QLocale::Yoruba;

static constexpr char iso639_2[3*lastLanguage + 4] = { // +3 for AnyLanguage and +1 for \0 at the end
    "und"
    "\0\0\0"
    "abk"
    "aar"
    "afr"
    "\0\0\0"
    "aka"
    "akk"
    "\0\0\0"
    "alb"
    "\0\0\0"
    "amh"
    "egy"
    "grc"
    "ara"
    "arg"
    "arc"
    "arm"
    "asm"
    "ast"
    "\0\0\0"
    "\0\0\0"
    "ava"
    "ave"
    "aym"
    "aze"
    "\0\0\0"
    "ban"
    "bam"
    "\0\0\0"
    "ben"
    "bas"
    "bak"
    "baq"
    "btk"
    "bel"
    "bem"
    "\0\0\0"
    "bho"
    "bis"
    "byn"
    "\0\0\0"
    "bos"
    "bre"
    "bug"
    "bul"
    "bur"
    "\0\0\0"
    "cat"
    "ceb"
    "zgh"
    "\0\0\0"
    "\0\0\0"
    "cha"
    "che"
    "chr"
    "\0\0\0"
    "\0\0\0"
    "chi"
    "chu"
    "chv"
    "\0\0\0"
    "cop"
    "cor"
    "cos"
    "cre"
    "hrv"
    "cze"
    "dan"
    "div"
    "doi"
    "dua"
    "dut"
    "dzo"
    "\0\0\0"
    "eng"
    "myv"
    "epo"
    "est"
    "ewe"
    "ewo"
    "fao"
    "fij"
    "fil"
    "fin"
    "fre"
    "fur"
    "ful"
    "gla"
    "gaa"
    "glg"
    "lug"
    "gez"
    "geo"
    "ger"
    "got"
    "gre"
    "grn"
    "guj"
    "\0\0\0"
    "hat"
    "hau"
    "haw"
    "heb"
    "her"
    "hin"
    "hmo"
    "hun"
    "ice"
    "ido"
    "ibo"
    "smn"
    "ind"
    "inh"
    "ina"
    "ile"
    "iku"
    "ipk"
    "gle"
    "ita"
    "jpn"
    "jav"
    "\0\0\0"
    "\0\0\0"
    "\0\0\0"
    "kab"
    "\0\0\0"
    "kal"
    "\0\0\0"
    "kam"
    "kan"
    "kau"
    "kas"
    "kaz"
    "\0\0\0"
    "khm"
    "\0\0\0"
    "\0\0\0"
    "kin"
    "kom"
    "kon"
    "kok"
    "kor"
    "\0\0\0"
    "\0\0\0"
    "\0\0\0"
    "kpe"
    "kua"
    "kur"
    "\0\0\0"
    "kir"
    "\0\0\0"
    "\0\0\0"
    "lao"
    "lat"
    "lav"
    "lez"
    "\0\0\0"
    "lin"
    "\0\0\0"
    "lit"
    "jbo"
    "dsb"
    "\0\0\0"
    "lub"
    "smj"
    "luo"
    "ltz"
    "\0\0\0"
    "mac"
    "\0\0\0"
    "mai"
    "\0\0\0"
    "\0\0\0"
    "mlg"
    "mal"
    "may"
    "mlt"
    "man"
    "mni"
    "glv"
    "mao"
    "arn"
    "mar"
    "mah"
    "mas"
    "\0\0\0"
    "men"
    "\0\0\0"
    "\0\0\0"
    "moh"
    "mon"
    "\0\0\0"
    "\0\0\0"
    "\0\0\0"
    "\0\0\0"
    "\0\0\0"
    "nav"
    "ndo"
    "nep"
    "\0\0\0"
    "\0\0\0"
    "\0\0\0"
    "\0\0\0"
    "nqo"
    "\0\0\0"
    "sme"
    "nso"
    "nde"
    "nob"
    "nno"
    "\0\0\0"
    "nya"
    "nyn"
    "oci"
    "ori"
    "oji"
    "sga"
    "non"
    "peo"
    "orm"
    "osa"
    "oss"
    "pal"
    "pau"
    "pli"
    "pap"
    "pus"
    "per"
    "phn"
    "pol"
    "por"
    "\0\0\0"
    "pan"
    "que"
    "rom"
    "roh"
    "\0\0\0"
    "run"
    "rus"
    "\0\0\0"
    "\0\0\0"
    "\0\0\0"
    "\0\0\0"
    "smo"
    "sag"
    "\0\0\0"
    "san"
    "sat"
    "srd"
    "\0\0\0"
    "\0\0\0"
    "srp"
    "\0\0\0"
    "sna"
    "\0\0\0"
    "scn"
    "sid"
    "\0\0\0"
    "snd"
    "sin"
    "sms"
    "slo"
    "slv"
    "\0\0\0"
    "som"
    "\0\0\0"
    "sma"
    "sot"
    "nbl"
    "spa"
    "zgh"
    "sun"
    "swa"
    "ssw"
    "swe"
    "gsw"
    "syr"
    "\0\0\0"
    "tah"
    "\0\0\0"
    "\0\0\0"
    "tgk"
    "tam"
    "\0\0\0"
    "\0\0\0"
    "tat"
    "tel"
    "\0\0\0"
    "tha"
    "tib"
    "tig"
    "tir"
    "\0\0\0"
    "tpi"
    "ton"
    "tso"
    "tsn"
    "tur"
    "tuk"
    "tvl"
    "\0\0\0"
    "uga"
    "ukr"
    "hsb"
    "urd"
    "uig"
    "uzb"
    "vai"
    "ven"
    "vie"
    "vol"
    "\0\0\0"
    "wln"
    "\0\0\0"
    "\0\0\0"
    "wel"
    "\0\0\0"
    "\0\0\0"
    "wal"
    "wol"
    "xho"
    "\0\0\0"
    "yid"
    "yor"
};
static_assert(iso639_2[lastLanguage * 3 + 2]);

static const struct {
    QLocale::Language lang;
    const char *tag;
} duplicatedTags[] = {
    { QLocale::Albanian, "sqi" },
    { QLocale::Armenian, "hye" },
    { QLocale::Basque, "eus" },
    { QLocale::Burmese, "mya" },
    { QLocale::Chinese, "zho" },
    { QLocale::Czech, "ces" },
    { QLocale::Dutch, "nld" },
    { QLocale::French, "fra" },
    { QLocale::Georgian, "kat" },
    { QLocale::German, "deu" },
    { QLocale::Greek, "ell" },
    { QLocale::Icelandic, "isl" },
    { QLocale::Macedonian, "mkd" },
    { QLocale::Malay, "msa" },
    { QLocale::Maori, "mri" },
    { QLocale::NorwegianBokmal, "nor" },
    { QLocale::Persian, "fas" },
    { QLocale::Slovak, "slk" },
    { QLocale::Tibetan, "bod" },
    { QLocale::Welsh, "cym" },
    { QLocale::AnyLanguage, nullptr }
};

QLocale::Language QtMultimediaPrivate::fromIso639(const char *tag)
{
    if (!tag)
        return QLocale::AnyLanguage;
    qsizetype l = (qsizetype)strnlen(tag, 4);
    if (l == 3) {
        for (int i = 0; i <= lastLanguage; ++i) {
            const char *isoTag = iso639_2 + 3 * i;
            if (tag[0] == isoTag[0] && tag[1] == isoTag[1] && tag[2] == isoTag[2])
                return QLocale::Language(i);
        }
    }
    // check duplicated languages. ISO639-2 has two tags (an international and a local
    // one) for some of the languages. We can handle the local tag for decoding but will
    // never set it when writing.
    auto *e = duplicatedTags;
    while (e->tag) {
        if (!strcmp(e->tag, tag))
            return e->lang;
        ++e;
    }

    // fallback, let's try if QLocale can handle it
    return QLocale::codeToLanguage(QString::fromLatin1(tag));
}

QByteArray QtMultimediaPrivate::toIso639(QLocale::Language language)
{
    if (language <= lastLanguage) {
        const char *tag = iso639_2 + 3 * int(language);
        if (*tag)
            return QByteArray(tag, 3);
    }

    QString ltag = QLocale::languageToCode(language);
    if (!ltag.isEmpty())
        return ltag.toLatin1();
    return "und";
}

QT_END_NAMESPACE

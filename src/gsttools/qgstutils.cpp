/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstutils_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qvariant.h>
#include <QtCore/qsize.h>

QT_BEGIN_NAMESPACE

//internal
static void addTagToMap(const GstTagList *list,
                        const gchar *tag,
                        gpointer user_data)
{
    QMap<QByteArray, QVariant> *map = reinterpret_cast<QMap<QByteArray, QVariant>* >(user_data);

    GValue val;
    val.g_type = 0;
    gst_tag_list_copy_value(&val,list,tag);

    switch( G_VALUE_TYPE(&val) ) {
        case G_TYPE_STRING:
        {
            const gchar *str_value = g_value_get_string(&val);
            map->insert(QByteArray(tag), QString::fromUtf8(str_value));
            break;
        }
        case G_TYPE_INT:
            map->insert(QByteArray(tag), g_value_get_int(&val));
            break;
        case G_TYPE_UINT:
            map->insert(QByteArray(tag), g_value_get_uint(&val));
            break;
        case G_TYPE_LONG:
            map->insert(QByteArray(tag), qint64(g_value_get_long(&val)));
            break;
        case G_TYPE_BOOLEAN:
            map->insert(QByteArray(tag), g_value_get_boolean(&val));
            break;
        case G_TYPE_CHAR:
            map->insert(QByteArray(tag), g_value_get_char(&val));
            break;
        case G_TYPE_DOUBLE:
            map->insert(QByteArray(tag), g_value_get_double(&val));
            break;
        default:
            // GST_TYPE_DATE is a function, not a constant, so pull it out of the switch
            if (G_VALUE_TYPE(&val) == GST_TYPE_DATE) {
                const GDate *date = gst_value_get_date(&val);
                if (g_date_valid(date)) {
                    int year = g_date_get_year(date);
                    int month = g_date_get_month(date);
                    int day = g_date_get_day(date);
                    map->insert(QByteArray(tag), QDate(year,month,day));
                    if (!map->contains("year"))
                        map->insert("year", year);
                }
            } else if (G_VALUE_TYPE(&val) == GST_TYPE_FRACTION) {
                int nom = gst_value_get_fraction_numerator(&val);
                int denom = gst_value_get_fraction_denominator(&val);

                if (denom > 0) {
                    map->insert(QByteArray(tag), double(nom)/denom);
                }
            }
            break;
    }

    g_value_unset(&val);
}

/*!
  Convert GstTagList structure to QMap<QByteArray, QVariant>.

  Mapping to int, bool, char, string, fractions and date are supported.
  Fraction values are converted to doubles.
*/
QMap<QByteArray, QVariant> QGstUtils::gstTagListToMap(const GstTagList *tags)
{
    QMap<QByteArray, QVariant> res;
    gst_tag_list_foreach(tags, addTagToMap, &res);

    return res;
}

/*!
  Returns resolution of \a caps.
  If caps doesn't have a valid size, and ampty QSize is returned.
*/
QSize QGstUtils::capsResolution(const GstCaps *caps)
{
    QSize size;

    if (caps) {
        const GstStructure *structure = gst_caps_get_structure(caps, 0);
        gst_structure_get_int(structure, "width", &size.rwidth());
        gst_structure_get_int(structure, "height", &size.rheight());
    }

    return size;
}

/*!
  Returns aspect ratio corrected resolution of \a caps.
  If caps doesn't have a valid size, and ampty QSize is returned.
*/
QSize QGstUtils::capsCorrectedResolution(const GstCaps *caps)
{
    QSize size;

    if (caps) {
        const GstStructure *structure = gst_caps_get_structure(caps, 0);
        gst_structure_get_int(structure, "width", &size.rwidth());
        gst_structure_get_int(structure, "height", &size.rheight());

        gint aspectNum = 0;
        gint aspectDenum = 0;
        if (!size.isEmpty() && gst_structure_get_fraction(
                    structure, "pixel-aspect-ratio", &aspectNum, &aspectDenum)) {
            if (aspectDenum > 0)
                size.setWidth(size.width()*aspectNum/aspectDenum);
        }
    }

    return size;
}

QT_END_NAMESPACE

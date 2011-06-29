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

#ifndef QWMPGLOBAL_H
#define QWMPGLOBAL_H

#include <wmp.h>

#include <QtCore/qstring.h>
#include <QtCore/qurl.h>

const char *qwmp_error_string(HRESULT hr);

class QAutoBStr
{
public:
    inline QAutoBStr(const QString &string)
        : m_string(::SysAllocString(static_cast<const wchar_t *>(string.utf16())))
    {
    }

    inline QAutoBStr(const QUrl &url)
        : m_string(::SysAllocString(static_cast<const wchar_t *>(url.toString().utf16())))
    {
    }

    inline QAutoBStr(const wchar_t *string)
        : m_string(::SysAllocString(string))
    {
    }

    inline ~QAutoBStr()
    {
        ::SysFreeString(m_string);
    }

    inline operator BSTR() const { return m_string; }

private:
    BSTR m_string;
};



#endif

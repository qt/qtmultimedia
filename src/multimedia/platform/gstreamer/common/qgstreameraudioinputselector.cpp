/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include "qgstreameraudioinputselector_p.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>

#include <gst/gst.h>

#include "qgstutils_p.h"

QGstreamerAudioInputSelector::QGstreamerAudioInputSelector(QObject *parent)
    :QAudioInputSelectorControl(parent)
{
    update();
}

QGstreamerAudioInputSelector::~QGstreamerAudioInputSelector()
{
}

QList<QString> QGstreamerAudioInputSelector::availableInputs() const
{
    return m_names;
}

QString QGstreamerAudioInputSelector::inputDescription(const QString& name) const
{
    QString desc;

    for (int i = 0; i < m_names.size(); i++) {
        if (m_names.at(i).compare(name) == 0) {
            desc = m_descriptions.at(i);
            break;
        }
    }
    return desc;
}

QString QGstreamerAudioInputSelector::defaultInput() const
{
    return m_defaultInput;
}

QString QGstreamerAudioInputSelector::activeInput() const
{
    return m_audioInput;
}

void QGstreamerAudioInputSelector::setActiveInput(const QString& name)
{
    if (m_audioInput.compare(name) != 0) {
        m_audioInput = name;
        emit activeInputChanged(name);
    }
}

void QGstreamerAudioInputSelector::update()
{
    QGstUtils::initializeGst();

    m_names.clear();
    m_descriptions.clear();

    const auto sources = QGstUtils::audioSources();
    for (auto *d : sources) {
        auto *properties = gst_device_get_properties(d);
        if (properties) {
            auto *klass = gst_structure_get_string(properties, "device.class");
            if (strcmp(klass, "monitor")) {
                auto *desc = gst_device_get_display_name(d);
                QString description = QString::fromUtf8(desc);
                g_free(desc);
                m_descriptions << description;

                auto *name = gst_structure_get_string(properties, "sysfs.path"); // ### Should this rather be "device.bus_path"?
                m_names << QString::fromLatin1(name);
                gboolean def;
                if (gst_structure_get_boolean(properties, "is-default", &def) && def)
                    m_defaultInput = QString::fromLatin1(name);
            }

            gst_structure_free(properties);
        }
    }

    if (m_names.size() > 0)
        m_audioInput = m_names.at(0);
}

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

#include "qt7playercontrol.h"
#include "qt7playersession.h"
#include <QtCore/qdebug.h>

QT_USE_NAMESPACE

/*
QT7VideoOutputControl::QT7VideoOutputControl(QObject *parent)
   :QVideoOutputControl(parent),
    m_session(0),
    m_output(QVideoOutputControl::NoOutput)
{
}

QT7VideoOutputControl::~QT7VideoOutputControl()
{
}

void QT7VideoOutputControl::setSession(QT7PlayerSession *session)
{
    m_session = session;
}

QList<QVideoOutputControl::Output> QT7VideoOutputControl::availableOutputs() const
{
    return m_outputs;
}

void QT7VideoOutputControl::enableOutput(QVideoOutputControl::Output output)
{
    if (!m_outputs.contains(output))
        m_outputs.append(output);
}

QVideoOutputControl::Output QT7VideoOutputControl::output() const
{
    return m_output;
}

void QT7VideoOutputControl::setOutput(Output output)
{
    if (m_output != output) {
        m_output = output;
        Q_EMIT videoOutputChanged(m_output);
    }
}

#include "moc_qt7videooutputcontrol.cpp"

*/

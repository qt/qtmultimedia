/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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
#include <QtCore/qcoreapplication.h>
#include <QtTest/QtTest>

#include "tst_qradiotuner.h"

#ifdef Q_OS_SYMBIAN
#include "tst_qradiotuner_xa.h"
#include "tst_qradiotuner_s60.h"
#endif

int main(int argc, char**argv)
{
    QApplication app(argc,argv);
    int ret;
    tst_QRadioTuner test_api;
    ret = QTest::qExec(&test_api, argc, argv);
#ifdef Q_OS_SYMBIAN
    char *new_argv[3];
    QString str = "C:\\data\\" + QFileInfo(QCoreApplication::applicationFilePath()).baseName() + "_xa.log";
    QByteArray   bytes  = str.toAscii();
    char arg1[] = "-o";
    new_argv[0] = argv[0];
    new_argv[1] = arg1;
    new_argv[2] = bytes.data();
    tst_QXARadio_xa test_xa;
    ret = QTest::qExec(&test_xa, 3, new_argv);
    char *new_argv1[3];
    QString str1 = "C:\\data\\" + QFileInfo(QCoreApplication::applicationFilePath()).baseName() + "_s60.log";
    QByteArray   bytes1  = str1.toAscii();
    char arg2[] = "-o";
    new_argv1[0] = argv[0];
    new_argv1[1] = arg2;
    new_argv1[2] = bytes1.data();
    tst_QRadioTuner_s60 test_s60;
    ret = QTest::qExec(&test_s60,  3, new_argv1);

#endif
    return ret;
}

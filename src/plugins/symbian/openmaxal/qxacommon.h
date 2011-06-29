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

#ifndef QXACOMMON_H
#define QXACOMMON_H

#if defined(PLUGIN_QT_TRACE_ENABLED) \
    || defined(PLUGIN_QT_SIGNAL_EMIT_TRACE_ENABLED) \
    || defined(PROFILE_TIME) \
    || defined(PROFILE_RAM_USAGE) \
    || defined(PROFILE_HEAP_USAGE)
#   include <QDebug>
#endif /* PLUGIN_QT_TRACE_ENABLED */

#ifdef PROFILE_RAM_USAGE
#   include <hal.h>
#endif


#ifdef PLUGIN_QT_TRACE_ENABLED
#   define QT_TRACE_FUNCTION_ENTRY qDebug() << __PRETTY_FUNCTION__ << ">"
#   define QT_TRACE_FUNCTION_EXIT qDebug() << __PRETTY_FUNCTION__ << "<"
#   define QT_TRACE_FUNCTION_ENTRY_EXIT qDebug() << __PRETTY_FUNCTION__ << "><"
#   define QT_TRACE1(v1) qDebug() << v1
#   define QT_TRACE2(v1, v2) qDebug() << v1 << v2
#else
#   define QT_TRACE_FUNCTION_ENTRY
#   define QT_TRACE_FUNCTION_EXIT
#   define QT_TRACE_FUNCTION_ENTRY_EXIT
#   define QT_TRACE1(v1)
#   define QT_TRACE2(v1, v2)
#endif /*PLUGIN_QT_TRACE_ENABLED*/

#ifdef PLUGIN_QT_SIGNAL_EMIT_TRACE_ENABLED
#   define SIGNAL_EMIT_TRACE1(v1) qDebug() << __PRETTY_FUNCTION__ << v1
#else
#   define SIGNAL_EMIT_TRACE1(v1)
#endif /*PLUGIN_QT_SIGNAL_EMIT_TRACE_ENABLED*/

#ifdef PROFILE_TIME_ELAPSED
#   define TAG_TIME_PROFILING_BEGIN \
        TTime beginProfilingTime; \
        beginProfilingTime.HomeTime()

#   define TAG_TIME_PROFILING_END \
        TTime endProfilingTime; \
        endProfilingTime.HomeTime(); \
        TTimeIntervalMicroSeconds diffInMicroSecs = endProfilingTime.MicroSecondsFrom(beginProfilingTime)

#   define QT_PRINT_TO_CONSOLE_TIME_DIFF \
        qDebug() << "VPROFILEDAT: " << __PRETTY_FUNCTION__ << ": Time taken " << diffInMicroSecs.Int64() << " microseconds"
#else /* Empty macros */
#   define TAG_TIME_PROFILING_BEGIN
#   define TAG_TIME_PROFILING_END
#   define QT_PRINT_TO_CONSOLE_TIME_DIFF
#endif /*PROFILE_TIME_ELAPSED*/

#ifdef PROFILE_RAM_USAGE
#   define TAG_RAM_PROFILING_BEGIN \
        TInt beginProfilingRAM; \
        TInt err1 = HAL::Get(HALData::EMemoryRAMFree, beginProfilingRAM)

#   define TAG_RAM_PROFILING_END \
        TInt endProfilingRAM; \
        TInt err2 = HAL::Get(HALData::EMemoryRAMFree, endProfilingRAM)

#   define QT_PRINT_TO_CONSOLE_RAM_DIFF \
        if ((err1 == KErrNone) && (err2 == KErrNone)) \
            { \
            TInt diffRAM = (beginProfilingRAM - endProfilingRAM); \
            if ( diffRAM > 0 ) \
                { \
                qDebug() << "VPROFILEDAT: " << __PRETTY_FUNCTION__ << ": " << diffRAM << " bytes of RAM used"; \
                } \
            else \
                { \
                qDebug() << "VPROFILEDAT: " << __PRETTY_FUNCTION__ << ": " << -(diffRAM) << " bytes of RAM freed"; \
                } \
            } \
        else \
            { \
            qDebug() << "VPROFILEDAT: " << __PRETTY_FUNCTION__ << "Error1[" << err1 << "] Error2[" << err2; \
            }

#else /* Empty macros */
#   define TAG_RAM_PROFILING_BEGIN
#   define TAG_RAM_PROFILING_END
#   define QT_PRINT_TO_CONSOLE_RAM_DIFF
#endif /*PROFILE_RAM_USAGE*/

#ifdef PROFILE_HEAP_USAGE
#   define TAG_DEFAULT_HEAP_PROFILING_BEGIN \
        TInt beginProfilingHEAPBiggestBlock; \
        TInt beginProfilingHEAP = User::Available(beginProfilingHEAPBiggestBlock) \

#   define TAG_DEFAULT_HEAP_PROFILING_END \
        TInt endProfilingHEAPBiggestBlock; \
        TInt endProfilingHEAP = User::Available(endProfilingHEAPBiggestBlock) \

#   define QT_PRINT_TO_CONSOLE_HEAP_DIFF \
        TInt diffHEAP = beginProfilingHEAP - endProfilingHEAP; \
        if ( diffHEAP > 0 ) \
            { \
            qDebug() << "VPROFILEDAT: " << __PRETTY_FUNCTION__ << ": " << diffHEAP << " bytes in default HEAP used"; \
            } \
        else \
            { \
            qDebug() << "VPROFILEDAT: " << __PRETTY_FUNCTION__ << ": " << -(diffHEAP) << " bytes in default HEAP freed"; \
            }
#else /* Empty macros */
#   define TAG_DEFAULT_HEAP_PROFILING_BEGIN
#   define TAG_DEFAULT_HEAP_PROFILING_END
#   define QT_PRINT_TO_CONSOLE_HEAP_DIFF
#endif /*PROFILE_HEAP_USAGE*/

/* This macro checks p pointer for null. If it is, returns value 's' from
 * function immediately.
 */
#define RET_s_IF_p_IS_NULL(p, s) \
    if (p == NULL) { \
        return s; \
        }

/* This macro checks p pointer for null. If it is, returns from function
 * immediately.
 */
#define RET_IF_p_IS_NULL(p) \
    if (p == NULL) { \
        return; \
        }

/* This macro checks p pointer for null. If it is, emits an error signal
 * error(QMediaPlayer::ResourceError, tr("Resource Error"));
 * and returns value 's' from function immediately.
 */
#define RET_s_IF_p_IS_NULL_EMIT_PLAYER_RESOURCE_ERROR(p, s) \
    if (p == NULL) { \
        emit error(QMediaPlayer::ResourceError, tr("Resource Error")); \
        SIGNAL_EMIT_TRACE1("emit error(QMediaPlayer::ResourceError, tr(\"Resource Error\"))"); \
        return s; \
        }

/* This macro checks p pointer for null. If it is, emits an error signal
 * error(QMediaPlayer::ResourceError, tr("Resource Error"));
 * and returns from function immediately.
 */
#define RET_IF_p_IS_NULL_EMIT_PLAYER_RESOURCE_ERROR(p) \
    if (p == NULL) { \
        emit error(QMediaPlayer::ResourceError, tr("Resource Error")); \
        SIGNAL_EMIT_TRACE1("emit error(QMediaPlayer::ResourceError, tr(\"Resource Error\"))"); \
        return; \
        }

/* This macro checks p pointer for null. If it is, emits an error signal
 * error(QMediaPlayer::ResourceError, tr("Resource Error"));
 * and returns from function immediately.
 */
#define RET_IF_ERROR(p) \
    if (p != KErrNone) { \
        emit error(QMediaPlayer::ResourceError, tr("Resource Error")); \
        SIGNAL_EMIT_TRACE1("emit error(QMediaPlayer::ResourceError, tr(\"Resource Error\"))"); \
        return; \
        }

#endif /* QXACOMMON_H */

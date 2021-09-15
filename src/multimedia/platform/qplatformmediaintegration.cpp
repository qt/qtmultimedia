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

#include <qtmultimediaglobal_p.h>
#include "qplatformmediaintegration_p.h"
#include <qatomic.h>
#include <qmutex.h>
#include <qplatformaudioinput_p.h>
#include <qplatformaudiooutput_p.h>

#if QT_CONFIG(gstreamer)
#include <private/qgstreamerintegration_p.h>
using PlatformIntegration = QGstreamerIntegration;
#elif QT_CONFIG(pulseaudio)
#include <private/qpulseaudiointegration_p.h>
using PlatformIntegration = QPulseAudioIntegration;
#elif QT_CONFIG(alsa)
#include <private/qalsaintegration_p.h>
using PlatformIntegration = QAlsaIntegration;
#elif QT_CONFIG(avfoundation)
#include <private/qdarwinintegration_p.h>
using PlatformIntegration = QDarwinIntegration;
#elif QT_CONFIG(wmf)
#include <private/qwindowsintegration_p.h>
using PlatformIntegration = QWindowsMediaIntegration;
#elif defined(Q_OS_ANDROID)
#include <private/qandroidintegration_p.h>
using PlatformIntegration = QAndroidIntegration;
#elif defined(Q_OS_WASM)
#include <private/qwasmmediaintegration_p.h>
using PlatformIntegration = QWasmMediaIntegration;
#else
class QDummyIntegration : public QPlatformMediaIntegration
{
public:
    QDummyIntegration() { qFatal("QtMultimedia is not currently supported on this platform or compiler."); }
    QPlatformMediaDevices *devices() override { return nullptr; }
    QPlatformMediaFormatInfo *formatInfo() override { return nullptr; }
};
using PlatformIntegration = QDummyIntegration;
#endif

QT_BEGIN_NAMESPACE

namespace {
struct Holder {
    ~Holder()
    {
        QMutexLocker locker(&mutex);
        delete nativeInstance;
        nativeInstance = nullptr;
        instance = nullptr;
    }
    QBasicMutex mutex;
    QPlatformMediaIntegration *instance = nullptr;
    QAtomicPointer<QPlatformMediaIntegration> nativeInstance = nullptr;
} holder;

}

QPlatformMediaIntegration *QPlatformMediaIntegration::instance()
{
    if (!holder.nativeInstance.loadRelaxed()) {
        QMutexLocker locker(&holder.mutex);
        if (!holder.nativeInstance.loadAcquire())
            holder.nativeInstance.storeRelease(new PlatformIntegration);
    }
    if (!holder.instance)
        holder.instance = holder.nativeInstance.loadRelaxed();
    return holder.instance;
}

/*
    This API is there to be able to test with a mock backend.
*/
void QPlatformMediaIntegration::setIntegration(QPlatformMediaIntegration *integration)
{
    holder.instance = integration;
}

QPlatformAudioInput *QPlatformMediaIntegration::createAudioInput(QAudioInput *q)
{
    return new QPlatformAudioInput(q);
}

QPlatformAudioOutput *QPlatformMediaIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QPlatformAudioOutput(q);
}

QPlatformMediaIntegration::~QPlatformMediaIntegration()
= default;

QT_END_NAMESPACE

/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <private/qgstutils_p.h>

#include <gst/gstplugin.h>
extern "C" {
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(playback);
GST_PLUGIN_STATIC_DECLARE(videotestsrc);
GST_PLUGIN_STATIC_DECLARE(audioconvert);
GST_PLUGIN_STATIC_DECLARE(videoconvert);
GST_PLUGIN_STATIC_DECLARE(autodetect);
GST_PLUGIN_STATIC_DECLARE(gio);
GST_PLUGIN_STATIC_DECLARE(soup);
GST_PLUGIN_STATIC_DECLARE(app);
GST_PLUGIN_STATIC_DECLARE(isomp4);
GST_PLUGIN_STATIC_DECLARE(avi);
GST_PLUGIN_STATIC_DECLARE(ogg);
GST_PLUGIN_STATIC_DECLARE(vorbis);
GST_PLUGIN_STATIC_DECLARE(tcp);
GST_PLUGIN_STATIC_DECLARE(audioparsers);
GST_PLUGIN_STATIC_DECLARE(theora);
GST_PLUGIN_STATIC_DECLARE(mpg123);
GST_PLUGIN_STATIC_DECLARE(typefindfunctions);
GST_PLUGIN_STATIC_DECLARE(x264);
GST_PLUGIN_STATIC_DECLARE(libav);
};

int main(int argc, char *argv[])
{
    QGstUtils::initializeGst();

    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(playback);
    GST_PLUGIN_STATIC_REGISTER(videotestsrc);
    GST_PLUGIN_STATIC_REGISTER(audioconvert);
    GST_PLUGIN_STATIC_REGISTER(videoconvert);
    GST_PLUGIN_STATIC_REGISTER(autodetect);
    GST_PLUGIN_STATIC_REGISTER(soup);
    GST_PLUGIN_STATIC_REGISTER(gio);
    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(isomp4);
    GST_PLUGIN_STATIC_REGISTER(avi);
    GST_PLUGIN_STATIC_REGISTER(ogg);
    GST_PLUGIN_STATIC_REGISTER(vorbis);
    GST_PLUGIN_STATIC_REGISTER(tcp);
    GST_PLUGIN_STATIC_REGISTER(audioparsers);
    GST_PLUGIN_STATIC_REGISTER(theora);
    GST_PLUGIN_STATIC_REGISTER(mpg123);
    GST_PLUGIN_STATIC_REGISTER(typefindfunctions);
    GST_PLUGIN_STATIC_REGISTER(x264);
    GST_PLUGIN_STATIC_REGISTER(libav);

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

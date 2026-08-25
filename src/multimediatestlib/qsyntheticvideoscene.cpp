// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qsyntheticvideoscene_p.h"

#include <QtGui/qpainter.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// The map is sent over D-Bus, so it may only contain types QtDBus can marshal --
// QSize, QRect and QColor are not among them. Everything is flattened to ints and
// colour names.
QVariantMap SyntheticVideoScene::toVariantMap() const
{
    return QVariantMap{
        { u"frameWidth"_s, frameSize.width() },
        { u"frameHeight"_s, frameSize.height() },
        { u"patternX"_s, patternRect.x() },
        { u"patternY"_s, patternRect.y() },
        { u"patternWidth"_s, patternRect.width() },
        { u"patternHeight"_s, patternRect.height() },
        { u"backgroundColor"_s, backgroundColor.name(QColor::HexArgb) },
        { u"firstColor"_s, firstColor.name(QColor::HexArgb) },
        { u"secondColor"_s, secondColor.name(QColor::HexArgb) },
        { u"animated"_s, animated },
        { u"scale"_s, scale },
    };
}

SyntheticVideoScene SyntheticVideoScene::fromVariantMap(const QVariantMap &map)
{
    const auto intOr = [&](const QString &key, int fallback) {
        const auto it = map.find(key);
        return it == map.end() ? fallback : it->toInt();
    };
    const auto colorOr = [&](const QString &key, QColor fallback) {
        const auto it = map.find(key);
        if (it == map.end())
            return fallback;
        const QColor color(it->toString());
        return color.isValid() ? color : fallback;
    };

    SyntheticVideoScene scene;
    scene.frameSize = QSize(intOr(u"frameWidth"_s, scene.frameSize.width()),
                            intOr(u"frameHeight"_s, scene.frameSize.height()));
    scene.patternRect = QRect(intOr(u"patternX"_s, scene.patternRect.x()),
                              intOr(u"patternY"_s, scene.patternRect.y()),
                              intOr(u"patternWidth"_s, scene.patternRect.width()),
                              intOr(u"patternHeight"_s, scene.patternRect.height()));
    scene.backgroundColor = colorOr(u"backgroundColor"_s, scene.backgroundColor);
    scene.firstColor = colorOr(u"firstColor"_s, scene.firstColor);
    scene.secondColor = colorOr(u"secondColor"_s, scene.secondColor);
    if (const auto it = map.find(u"animated"_s); it != map.end())
        scene.animated = it->toBool();
    if (const auto it = map.find(u"scale"_s); it != map.end())
        scene.scale = it->toReal();
    return scene;
}

void paintSyntheticVideoScene(QPainter &painter, const SyntheticVideoScene &scene, unsigned tick)
{
    QColor first = scene.firstColor;
    QColor second = scene.secondColor;
    if (scene.animated && (tick & 1u))
        std::swap(first, second);

    painter.fillRect(QRect(QPoint(0, 0), scene.frameSize), scene.backgroundColor);
    painter.fillRect(scene.patternRect, first);
    painter.fillRect(scene.insetRect(), second);
}

QImage renderSyntheticVideoScene(const SyntheticVideoScene &scene, unsigned tick)
{
    QImage image(scene.frameSize, QImage::Format_RGB32);
    QPainter painter(&image);
    paintSyntheticVideoScene(painter, scene, tick);
    return image;
}

QT_END_NAMESPACE

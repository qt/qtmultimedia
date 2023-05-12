// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCAPTURABLEWINDOW_H
#define QCAPTURABLEWINDOW_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QCapturableWindowPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QCapturableWindowPrivate, Q_MULTIMEDIA_EXPORT)

class QMediaCaptureSession;
class QWindowCapturePrivate;

class Q_MULTIMEDIA_EXPORT QCapturableWindow
{
    Q_GADGET
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(bool isValid READ isValid CONSTANT)
public:
    QCapturableWindow();

    ~QCapturableWindow();

    QCapturableWindow(const QCapturableWindow &other);

    QCapturableWindow(QCapturableWindow &&other) noexcept = default;

    QCapturableWindow& operator=(const QCapturableWindow &other);

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QCapturableWindow);

    void swap(QCapturableWindow &other) noexcept
    { d.swap(other.d); }

    bool operator==(const QCapturableWindow &other) const;

    bool operator!=(const QCapturableWindow &other) const;

    bool isValid() const;

    QString description() const;

private:
    QCapturableWindow(QCapturableWindowPrivate *capturablePrivate);

    QExplicitlySharedDataPointer<QCapturableWindowPrivate> d;
};

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, const QCapturableWindow &);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QCapturableWindow)

#endif // QCAPTURABLEWINDOW_H

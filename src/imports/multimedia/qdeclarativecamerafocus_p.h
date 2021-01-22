/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QDECLARATIVECAMERAFOCUS_H
#define QDECLARATIVECAMERAFOCUS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qabstractitemmodel.h>
#include <qcamera.h>
#include <qcamerafocus.h>
#include "qdeclarativecamera_p.h"

QT_BEGIN_NAMESPACE

class FocusZonesModel;
class QDeclarativeCamera;

class QDeclarativeCameraFocus : public QObject
{
    Q_OBJECT

    Q_PROPERTY(FocusMode focusMode READ focusMode WRITE setFocusMode NOTIFY focusModeChanged)
    Q_PROPERTY(QVariantList supportedFocusModes READ supportedFocusModes NOTIFY supportedFocusModesChanged REVISION 1)

    Q_PROPERTY(FocusPointMode focusPointMode READ focusPointMode WRITE setFocusPointMode NOTIFY focusPointModeChanged)
    Q_PROPERTY(QVariantList supportedFocusPointModes READ supportedFocusPointModes NOTIFY supportedFocusPointModesChanged REVISION 1)

    Q_PROPERTY(QPointF customFocusPoint READ customFocusPoint WRITE setCustomFocusPoint NOTIFY customFocusPointChanged)
    Q_PROPERTY(QObject *focusZones READ focusZones CONSTANT)

    Q_ENUMS(FocusMode)
    Q_ENUMS(FocusPointMode)
public:
    enum FocusMode {
        FocusManual = QCameraFocus::ManualFocus,
        FocusHyperfocal = QCameraFocus::HyperfocalFocus,
        FocusInfinity = QCameraFocus::InfinityFocus,
        FocusAuto = QCameraFocus::AutoFocus,
        FocusContinuous = QCameraFocus::ContinuousFocus,
        FocusMacro = QCameraFocus::MacroFocus
    };

    enum FocusPointMode {
        FocusPointAuto = QCameraFocus::FocusPointAuto,
        FocusPointCenter = QCameraFocus::FocusPointCenter,
        FocusPointFaceDetection = QCameraFocus::FocusPointFaceDetection,
        FocusPointCustom = QCameraFocus::FocusPointCustom
    };

    ~QDeclarativeCameraFocus();

    FocusMode focusMode() const;
    QVariantList supportedFocusModes() const;

    FocusPointMode focusPointMode() const;
    QVariantList supportedFocusPointModes() const;

    QPointF customFocusPoint() const;
    QAbstractListModel *focusZones() const;

#if QT_DEPRECATED_SINCE(5, 10)
    Q_INVOKABLE bool isFocusModeSupported(FocusMode mode) const;
    Q_INVOKABLE bool isFocusPointModeSupported(FocusPointMode mode) const;
#endif

public Q_SLOTS:
    void setFocusMode(FocusMode);
    void setFocusPointMode(FocusPointMode mode);
    void setCustomFocusPoint(const QPointF &point);

Q_SIGNALS:
    void focusModeChanged(FocusMode);
    void supportedFocusModesChanged();
    void focusPointModeChanged(FocusPointMode);
    void supportedFocusPointModesChanged();
    void customFocusPointChanged(const QPointF &);

private Q_SLOTS:
    void updateFocusZones();

private:
    friend class QDeclarativeCamera;
    QDeclarativeCameraFocus(QCamera *camera, QObject *parent = 0);

    QCameraFocus *m_focus;
    FocusZonesModel *m_focusZones;
};

class FocusZonesModel : public QAbstractListModel
{
Q_OBJECT
public:
    enum FocusZoneRoles {
        StatusRole = Qt::UserRole + 1,
        AreaRole
    };

    FocusZonesModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int,QByteArray> roleNames() const override;

public slots:
    void setFocusZones(const QCameraFocusZoneList &zones);

private:
    QCameraFocusZoneList m_focusZones;
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QT_PREPEND_NAMESPACE(QDeclarativeCameraFocus))

#endif

// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXCAMERA_H
#define QQNXCAMERA_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqnxcamerahandle_p.h"

#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qurl.h>

#include <camera/camera_api.h>
#include <camera/camera_3a.h>

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

class QQnxCameraFrameBuffer;
class QQnxMediaCaptureSession;
class QQnxVideoSink;

class QQnxCamera : public QObject
{
    Q_OBJECT
public:
    explicit QQnxCamera(camera_unit_t unit, QObject *parent = nullptr);
    ~QQnxCamera();

    camera_unit_t unit() const;

    QString name() const;

    bool isValid() const;

    bool isActive() const;
    void start();
    void stop();

    bool startVideoRecording(const QString &filename);
    void stopVideoRecording();

    bool setCameraFormat(uint32_t width, uint32_t height, double frameRate);

    bool isFocusModeSupported(camera_focusmode_t mode) const;
    bool setFocusMode(camera_focusmode_t mode);
    camera_focusmode_t focusMode() const;

    void setCustomFocusPoint(const QPointF &point);

    void setManualFocusStep(int step);
    int manualFocusStep() const;
    int maxFocusStep() const;

    QSize viewFinderSize() const;

    uint32_t minimumZoomLevel() const;
    uint32_t maximumZoomLevel() const;
    bool isSmoothZoom() const;
    double zoomRatio(uint32_t zoomLevel) const;
    bool setZoomFactor(uint32_t factor);

    void setEvOffset(float ev);

    uint32_t manualIsoSensitivity() const;
    void setManualIsoSensitivity(uint32_t value);
    void setManualExposureTime(double seconds);
    double manualExposureTime() const;

    void setWhiteBalanceMode(camera_whitebalancemode_t mode);
    camera_whitebalancemode_t whiteBalanceMode() const;

    void setManualWhiteBalance(uint32_t value);
    uint32_t manualWhiteBalance() const;

    bool hasFeature(camera_feature_t feature) const;

    camera_handle_t handle() const;

    QList<camera_vfmode_t> supportedVfModes() const;
    QList<camera_res_t> supportedVfResolutions() const;
    QList<camera_frametype_t> supportedVfFrameTypes() const;
    QList<camera_focusmode_t> supportedFocusModes() const;
    QList<double> specifiedVfFrameRates(camera_frametype_t frameType,
            camera_res_t resolution) const;

    QList<camera_frametype_t> supportedRecordingFrameTypes() const;

    QList<uint32_t> supportedWhiteBalanceValues() const;

    bool hasContinuousWhiteBalanceValues() const;

    static QList<camera_unit_t> supportedUnits();

    std::unique_ptr<QQnxCameraFrameBuffer> takeCurrentFrame();

Q_SIGNALS:
    void focusModeChanged(camera_focusmode_t mode);
    void customFocusPointChanged(const QPointF &point);
    void minimumZoomFactorChanged(double factor);

    double maximumZoomFactorChanged(double factor);

    void frameAvailable();

private:
    struct FocusStep
    {
        int step; // current step
        int maxStep; // max supported step
    };

    FocusStep focusStep() const;

    struct VideoFormat
    {
        uint32_t width;
        uint32_t height;
        uint32_t rotation;
        double frameRate;
        camera_frametype_t frameType;
    };

    friend QDebug &operator<<(QDebug&, const VideoFormat&);

    VideoFormat vfFormat() const;
    void setVfFormat(const VideoFormat &format);

    VideoFormat recordingFormat() const;
    void setRecordingFormat(const VideoFormat &format);

    void updateZoomLimits();
    void updateSupportedWhiteBalanceValues();
    void setColorTemperatureInternal(unsigned temp);

    bool isVideoEncodingSupported() const;

    void handleVfBuffer(camera_buffer_t *buffer);

    // viewfinder callback
    void handleVfStatus(camera_devstatus_t status, uint16_t extraData);

    // our handler running on main thread
    Q_INVOKABLE void handleStatusChange(camera_devstatus_t status, uint16_t extraData);

    template <typename T, typename U>
    using QueryFuncPtr = camera_error_t (*)(camera_handle_t, U, U *, T *);

    template <typename T, typename U>
    QList<T> queryValues(QueryFuncPtr<T, U> func) const;

    static void viewfinderCallback(camera_handle_t handle,
            camera_buffer_t *buffer, void *arg);

    static void statusCallback(camera_handle_t handle, camera_devstatus_t status,
            uint16_t extraData, void *arg);

    QQnxMediaCaptureSession *m_session = nullptr;

    camera_unit_t m_cameraUnit = CAMERA_UNIT_NONE;

    QQnxCameraHandle m_handle;

    uint32_t m_minZoom = 0;
    uint32_t m_maxZoom = 0;

    QMutex m_currentFrameMutex;

    QList<uint32_t> m_supportedWhiteBalanceValues;

    std::unique_ptr<QQnxCameraFrameBuffer> m_currentFrame;

    std::optional<VideoFormat> m_originalVfFormat;

    bool m_viewfinderActive = false;
    bool m_recordingVideo = false;
    bool m_valid = false;
    bool m_smoothZoom = false;
    bool m_continuousWhiteBalanceValues = false;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(camera_devstatus_t)
Q_DECLARE_METATYPE(uint16_t)

#endif

// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMSURFACECAPTURE_H
#define QPLATFORMSURFACECAPTURE_H

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

#include <QtMultimedia/private/qerrorinfo_p.h>
#include <QtMultimedia/private/qplatformvideosource_p.h>
#include <QtMultimedia/qcapturablewindow.h>
#include <QtGui/qscreen.h>
#include <QtCore/qpointer.h>
#include <QtCore/private/qexpected_p.h>

#include <optional>
#include <variant>

QT_BEGIN_NAMESPACE

class QVideoFrame;

class Q_MULTIMEDIA_EXPORT QPlatformSurfaceCapture : public QPlatformVideoSource
{
    Q_OBJECT

public:
    enum class Error {
        NoError = 0,
        InternalError = 1,
        CapturingNotSupported = 2,
        CaptureFailed = 4,
        NotFound = 5,
    };

    using ScreenSource = QPointer<QScreen>;
    using WindowSource = QCapturableWindow;

    using Source = std::variant<ScreenSource, WindowSource>;

    struct ErrorDetails
    {
        Error error = Error::InternalError;
        // Description is user-facing. Emitted in errorOccurred.
        QString description;
    };
    using SourceSwitchResult = q23::expected<void, ErrorDetails>;

    explicit QPlatformSurfaceCapture(Source initialSource);

    // When capturing a Window, we should never include the drop shadow of that Window.
    // Ideally, when configured with QCapturableWindow constructed from a QWindow,
    // our output frames should always have sizes matching that QWindow.
    //
    // If the property ignoreCursor() is set to true, we should provide a hint for the backend
    // to not capture the cursor if possible.
    //
    // This method must be implemented as blocking. If the function fails for some reason,
    // the error property can not be set to NoError.
    void setActive(bool active) override;
    bool isActive() const override;

    // Changes the source. If we are active, the new source is captured right away,
    // see switchActiveSource(). If that fails, we go inactive and report the error.
    void setSource(Source source);

    template<typename Type>
    Type source() const {
        Q_ASSERT(std::holds_alternative<Type>(m_source));
        return *std::get_if<Type>(&m_source);
    }

    Source source() const { return m_source; }

    Error error() const;
    QString errorString() const final;

    void setFrameRate(std::optional<qreal>);
    [[nodiscard]] std::optional<qreal> frameRate() const;

    void setIgnoreCursor(bool);
    // Applied on next stream start.
    [[nodiscard]] bool ignoreCursor() const;

protected:
    virtual bool setActiveInternal(bool) = 0;

    // Switches an active capture over to newSource, which is always of the same kind
    // as the current source (window to window, or screen to screen).
    //
    // Presentation times must keep increasing across the switch.
    //
    // The implementatiom must not emit an end-of-stream frame when the switch occurs.
    //
    // This function blocks until the switch is complete. Preferably, no frames of
    // the old source are emitted after it returns.
    //
    // If the switch fails, the implementation may tear its resources down immediately
    // before returning.
    [[nodiscard]] virtual std::optional<SourceSwitchResult>
    switchActiveSource(const Source &newSource);

    bool checkScreenWithError(ScreenSource &screen);

public Q_SLOTS:
    void updateError(Error error, const QString &errorString);

Q_SIGNALS:
    void sourceChanged(WindowSource);
    void sourceChanged(ScreenSource);
    void errorOccurred(Error error, QString errorString);
    void frameRateChanged(std::optional<qreal>);

private:
    std::optional<qreal> m_frameRate;
    QErrorInfo<Error> m_error;
    Source m_source;
    bool m_active = false;

    // While not part of public API, this setting tends to impact
    // integration tests.
    bool m_ignoreCursor = false;
};

QT_END_NAMESPACE

#endif // QPLATFORMSURFACECAPTURE_H

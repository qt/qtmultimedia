// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMESSAGESPY_P_H
#define QMESSAGESPY_P_H

#include <QtCore/qloggingcategory.h>
#include <QtCore/qstring.h>
#include <QtCore/qspan.h>
#include <QtCore/qmutex.h>
#include <QtCore/qregularexpression.h>
#include <QtTest/qtest.h>

#include <QtCore/q20algorithm.h>
#include <QtCore/q20vector.h>
#include <bitset>
#include <chrono>
#include <memory>
#include <variant>
#include <vector>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

/*!
    \internal

    QLoggingCategoryEnabler temporarily enables a logging category for all
    message types, and restores the original state on destruction.

    Usage:
    \code
        QLoggingCategoryEnabler enable(qLcMyCategory());
        // all message types for qLcMyCategory are now active for this scope
    \endcode
*/
class QLoggingCategoryEnabler
{
    Q_DISABLE_COPY_MOVE(QLoggingCategoryEnabler)
public:
    explicit QLoggingCategoryEnabler(const QLoggingCategory &category)
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        : m_category(const_cast<QLoggingCategory &>(category))
    {
        for (int type = QtDebugMsg; type <= QtFatalMsg; ++type) {
            auto msgType = static_cast<QtMsgType>(type);
            m_wasEnabled[type] = category.isEnabled(msgType);
            if (!m_wasEnabled[type])
                m_category.setEnabled(msgType, true);
        }
    }

    ~QLoggingCategoryEnabler()
    {
        for (int type = QtDebugMsg; type <= QtFatalMsg; ++type) {
            auto msgType = static_cast<QtMsgType>(type);
            if (!m_wasEnabled[type])
                m_category.setEnabled(msgType, false);
        }
    }

private:
    QLoggingCategory &m_category;
    std::bitset<QtFatalMsg + 1> m_wasEnabled;
};

/*!
    \internal

    QMessageSpy intercepts Qt logging messages via qInstallMessageHandler.
    Messages matching a pending expect() are silently consumed; all other
    messages are forwarded to the previous handler (e.g. QTest's default).

    Usage:
    \code
        QMessageSpy spy;

        // Register expectation BEFORE triggering the message:
        auto token = spy.expect(QtDebugMsg, "expected message");
        triggerCodeThatLogs();
        QVERIFY(token.wait());

        // Regex variant:
        auto token2 = spy.expect(QtWarningMsg, QRegularExpression("file: .*\\.wav"));
        triggerCode();
        QVERIFY(token2.wait(2000ms));
    \endcode

    Multiple waiters are supported simultaneously.
*/
class QMessageSpy
{
    Q_DISABLE_COPY_MOVE(QMessageSpy)

    struct PendingExpect
    {
        const QtMsgType type;
        const std::variant<QString, QRegularExpression> matcher;
        bool matched = false;
    };

public:
    /*!
        \internal
        WaitToken is returned by expect(). Call wait() on it after the
        message-generating code runs.
    */
    class WaitToken
    {
    public:
        WaitToken() = delete;
        WaitToken(WaitToken &&) noexcept = default;

        [[nodiscard]] bool
        wait(std::chrono::milliseconds timeout = std::chrono::milliseconds{ 5000 }) const
        {
            return QTest::qWaitFor([this]() {
                return m_state->matched;
            }, timeout);
        }

        [[nodiscard]] bool matched() const { return m_state->matched; }

    private:
        friend class QMessageSpy;
        explicit WaitToken(std::shared_ptr<PendingExpect> state) : m_state(std::move(state))
        {
            Q_ASSERT(m_state);
        }
        const std::shared_ptr<PendingExpect> m_state;
    };

    explicit QMessageSpy(const QLoggingCategory &category)
        : QMessageSpy(QLatin1StringView(category.categoryName()))
    {
    }

    explicit QMessageSpy(QLatin1StringView category = {})
        : m_categoryFilter{
              category.isEmpty() ? std::nullopt : std::make_optional(QByteArray(category)),
          }
    {
        Q_ASSERT(!s_instance);
        s_instance = this;
        m_previousHandler = qInstallMessageHandler(messageHandler);
    }

    ~QMessageSpy()
    {
        qInstallMessageHandler(m_previousHandler);
        s_instance = nullptr;
    }

    /*!
        Register an expectation for a message of \a type matching \a message
        exactly. Returns a WaitToken; call wait() on it after triggering the
        message. The matching message is consumed (not forwarded to the previous
        handler).
    */
    [[nodiscard]] WaitToken expect(QtMsgType type, const QString &message)
    {
        auto state = std::make_shared<PendingExpect>(PendingExpect{ type, message, false });
        std::lock_guard lock(m_mutex);
        m_pending.push_back(state);
        return WaitToken{
            std::move(state),
        };
    }

    /*!
        Register an expectation for a message of \a type matching \a pattern.
    */
    [[nodiscard]] WaitToken expect(QtMsgType type, const QRegularExpression &pattern)
    {
        auto state =
                std::make_shared<PendingExpect>(PendingExpect{ type, pattern, false });
        std::lock_guard lock(m_mutex);
        m_pending.push_back(state);
        return WaitToken{
            std::move(state),
        };
    }

private:
    static bool matchesPending(PendingExpect &p, QtMsgType type, const QString &msg)
    {
        if (p.matched || p.type != type)
            return false;
        return std::visit([&](auto &m) -> bool {
            using T = std::decay_t<decltype(m)>;
            if constexpr (std::is_same_v<T, QString>)
                return m == msg;
            else
                return m.match(msg).hasMatch();
        }, p.matcher);
    }

    static void messageHandler(QtMsgType type, const QMessageLogContext &context,
                               const QString &msg)
    {
        Q_ASSERT(s_instance);

        if (!(s_instance->m_categoryFilter)
            || QByteArrayView{ context.category } == s_instance->m_categoryFilter) {
            std::lock_guard lock(s_instance->m_mutex);
            bool doCleanupOnExit = false;
            auto cleanup = qScopeGuard([&] {
                if (!doCleanupOnExit)
                    q20::erase_if(s_instance->m_pending, [](const std::weak_ptr<PendingExpect> &p) {
                        auto locked = p.lock();
                        return !locked || p.lock()->matched;
                    });
            });

            // Check pending expects against the raw message first; first match wins and is consumed
            for (std::weak_ptr p : s_instance->m_pending) {
                auto pending = p.lock();
                if (!pending) {
                    doCleanupOnExit = true;
                    continue;
                }
                if (matchesPending(*pending, type, msg)) {
                    pending->matched = true;
                    doCleanupOnExit = true;
                    return; // consumed — do NOT forward
                }
            }
        }

        // Not consumed — forward to previous handler
        if (s_instance->m_previousHandler)
            s_instance->m_previousHandler(type, context, msg);
    }

    static inline QMessageSpy *s_instance = nullptr;

    QtMessageHandler m_previousHandler = nullptr;
    QMutex m_mutex;
    const std::optional<QByteArray> m_categoryFilter;
    std::vector<std::weak_ptr<PendingExpect>> m_pending;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QMESSAGESPY_P_H

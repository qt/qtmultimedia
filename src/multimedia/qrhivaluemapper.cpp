// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhivaluemapper_p.h"

#include <qmutex.h>
#include <rhi/qrhi.h>
#include <q20vector.h>

#include <vector>

QT_BEGIN_NAMESPACE

// ensures thread-safe access to rhi cleanup handlers
class QRhiCallback::Manager : public std::enable_shared_from_this<Manager>
{
    using CallbackList = std::vector<std::weak_ptr<QRhiCallback>>;
    struct CallbacksItem
    {
        CallbackList callbacks;
        size_t lastValidCallbackCount = 1u;

        void addCallback(const std::weak_ptr<QRhiCallback> &cb)
        {
            Q_ASSERT(!cb.expired());

            // check periodically to ensure avg O(1)
            if (callbacks.size() > lastValidCallbackCount * 2) {
                q20::erase_if(callbacks, [](const auto &cb) {
                    return cb.expired();
                });
                lastValidCallbackCount = callbacks.size() + 1;
            }

            callbacks.push_back(cb);
        }
    };

public:
    void registerCallback(QRhi &rhi, const std::weak_ptr<QRhiCallback> &cb)
    {
        QMutexLocker locker(&m_mutex);
        auto [rhiIt, added] = m_rhiToCallbackItems.try_emplace(&rhi, CallbacksItem{});
        if (added)
            rhi.addCleanupCallback([instance = shared_from_this()](QRhi *rhi) {
                for (auto &weakCb : instance->extractCallbacks(rhi))
                    if (auto cb = weakCb.lock())
                        cb->onRhiCleanup(*rhi); // run outside the global mutex
            });

        rhiIt->second.addCallback(cb);
    }

private:
    CallbackList extractCallbacks(QRhi *rhi)
    {
        QMutexLocker locker(&m_mutex);
        auto it = m_rhiToCallbackItems.find(rhi);
        Q_ASSERT(it != m_rhiToCallbackItems.end());

        CallbackList result = std::move(it->second.callbacks);
        m_rhiToCallbackItems.erase(it);
        return result;
    }

private:
    std::unordered_map<QRhi *, CallbacksItem> m_rhiToCallbackItems;
    QBasicMutex m_mutex;
};

Q_GLOBAL_STATIC(std::shared_ptr<QRhiCallback::Manager>, rhiCallbacksStorage,
                std::make_shared<QRhiCallback::Manager>());

QRhiCallback::QRhiCallback() : m_manager(*rhiCallbacksStorage) { }

QRhiCallback::~QRhiCallback() = default; // must be out-of-line

void QRhiCallback::registerCallback(QRhi &rhi)
{
    m_manager->registerCallback(rhi, weak_from_this());
}

QT_END_NAMESPACE

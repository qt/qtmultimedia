// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android.multimedia.qffmpeg;

import java.security.KeyStore;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

import android.util.Log;

import org.qtproject.qt.android.UsedFromNativeCode;

// Provides Android's system trust anchors (DER-encoded) to the FFmpeg backend, so it can
// build a CA bundle for FFmpeg's tls_openssl protocol. Reimplements the logic of
// org.qtproject.qt.android.QtNative.getSSLCertificates(), which is private and unreachable
// from here, to avoid a new QtNetwork dependency.
class QtSslCertificates {

    private static final String TAG = "QtSslCertificates";

    @UsedFromNativeCode
    static byte[][] getCertificates() {
        ArrayList<byte[]> certificateList = new ArrayList<>();

        try {
            TrustManagerFactory factory =
                    TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
            factory.init((KeyStore) null);

            for (TrustManager manager : factory.getTrustManagers()) {
                if (manager instanceof X509TrustManager) {
                    X509TrustManager trustManager = (X509TrustManager) manager;

                    for (X509Certificate certificate : trustManager.getAcceptedIssuers())
                        certificateList.add(certificate.getEncoded());
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to get system CA certificates", e);
        }

        return certificateList.toArray(new byte[certificateList.size()][]);
    }
}

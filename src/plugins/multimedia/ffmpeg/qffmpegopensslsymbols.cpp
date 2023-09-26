// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qlibrary.h>

#include "qffmpegsymbolsresolveutils_p.h"

#include <QtCore/qglobal.h>
#include <qstringliteral.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static Libs loadLibs()
{
    Libs libs(2);
    libs[0] = std::make_unique<QLibrary>();
    libs[1] = std::make_unique<QLibrary>();

    const auto majorVersion = OPENSSL_VERSION_NUMBER >> 28;

    auto tryLoad = [&](QString sslName, QString cryptoName, auto version) {
        libs[0]->setFileNameAndVersion(sslName, version);
        libs[1]->setFileNameAndVersion(cryptoName, version);
        return LibSymbolsResolver::tryLoad(libs);
    };

// Due to binary compatibility issues between 1.x.x openssl version, let's try taking exact version
#if defined(SHLIB_VERSION_NUMBER)
    if (majorVersion <= 1 && tryLoad("ssl"_L1, "crypto"_L1, SHLIB_VERSION_NUMBER ""_L1))
        return libs;
#endif

// openssl on Android has specific suffixes
#if defined(Q_OS_ANDROID)
    {
        auto suffix = qEnvironmentVariable("ANDROID_OPENSSL_SUFFIX");
        if (suffix.isEmpty())
            suffix = QString("_"_L1) + QString::number(majorVersion);

        if (tryLoad("ssl"_L1 + suffix, "crypto"_L1 + suffix, -1))
            return libs;
    }
#endif

    if (tryLoad("ssl"_L1, "crypto"_L1, majorVersion))
        return libs;

    return {};
};

Q_GLOBAL_STATIC(LibSymbolsResolver, resolver, "OpenSsl", 75, loadLibs);

void resolveOpenSsl()
{
    resolver()->resolve();
}

QT_END_NAMESPACE

QT_USE_NAMESPACE

// BN functions

DEFINE_FUNC(BN_value_one, 0);
DEFINE_FUNC(BN_mod_word, 2);

DEFINE_FUNC(BN_div_word, 2)
DEFINE_FUNC(BN_mul_word, 2)
DEFINE_FUNC(BN_add_word, 2)
DEFINE_FUNC(BN_sub_word, 2)
DEFINE_FUNC(BN_set_word, 2)
DEFINE_FUNC(BN_new, 0)
DEFINE_FUNC(BN_cmp, 2)

DEFINE_FUNC(BN_free, 1);

DEFINE_FUNC(BN_copy, 2);

DEFINE_FUNC(BN_CTX_new, 0);

DEFINE_FUNC(BN_CTX_free, 1);
DEFINE_FUNC(BN_CTX_start, 1);

DEFINE_FUNC(BN_CTX_get, 1);
DEFINE_FUNC(BN_CTX_end, 1);

DEFINE_FUNC(BN_rand, 4);
DEFINE_FUNC(BN_mod_exp, 5);

DEFINE_FUNC(BN_num_bits, 1);
DEFINE_FUNC(BN_num_bits_word, 1);

DEFINE_FUNC(BN_bn2hex, 1);
DEFINE_FUNC(BN_bn2dec, 1);

DEFINE_FUNC(BN_hex2bn, 2);
DEFINE_FUNC(BN_dec2bn, 2);
DEFINE_FUNC(BN_asc2bn, 2);

DEFINE_FUNC(BN_bn2bin, 2);
DEFINE_FUNC(BN_bin2bn, 3);

// BIO-related functions

DEFINE_FUNC(BIO_new, 1);
DEFINE_FUNC(BIO_free, 1);

DEFINE_FUNC(BIO_read, 3, -1);
DEFINE_FUNC(BIO_write, 3, -1);
DEFINE_FUNC(BIO_s_mem, 0);

DEFINE_FUNC(BIO_set_data, 2);

DEFINE_FUNC(BIO_get_data, 1);
DEFINE_FUNC(BIO_set_init, 2);

DEFINE_FUNC(BIO_set_flags, 2);
DEFINE_FUNC(BIO_test_flags, 2);
DEFINE_FUNC(BIO_clear_flags, 2);

DEFINE_FUNC(BIO_meth_new, 2);
DEFINE_FUNC(BIO_meth_free, 1);

DEFINE_FUNC(BIO_meth_set_write, 2);
DEFINE_FUNC(BIO_meth_set_read, 2);
DEFINE_FUNC(BIO_meth_set_puts, 2);
DEFINE_FUNC(BIO_meth_set_gets, 2);
DEFINE_FUNC(BIO_meth_set_ctrl, 2);
DEFINE_FUNC(BIO_meth_set_create, 2);
DEFINE_FUNC(BIO_meth_set_destroy, 2);
DEFINE_FUNC(BIO_meth_set_callback_ctrl, 2);

// SSL functions

DEFINE_FUNC(SSL_CTX_new, 1);
DEFINE_FUNC(SSL_CTX_up_ref, 1);
DEFINE_FUNC(SSL_CTX_free, 1);

DEFINE_FUNC(SSL_new, 1);
DEFINE_FUNC(SSL_up_ref, 1);
DEFINE_FUNC(SSL_free, 1);

DEFINE_FUNC(SSL_accept, 1);
DEFINE_FUNC(SSL_stateless, 1);
DEFINE_FUNC(SSL_connect, 1);
DEFINE_FUNC(SSL_read, 3, -1);
DEFINE_FUNC(SSL_peek, 3);
DEFINE_FUNC(SSL_write, 3, -1);
DEFINE_FUNC(SSL_ctrl, 4);
DEFINE_FUNC(SSL_shutdown, 1);
DEFINE_FUNC(SSL_set_bio, 3);

// options are unsigned long in openssl 1.1.1, and uint64 in 3.x.x
DEFINE_FUNC(SSL_CTX_set_options, 2);

DEFINE_FUNC(SSL_get_error, 2);
DEFINE_FUNC(SSL_CTX_load_verify_locations, 3, -1);

DEFINE_FUNC(SSL_CTX_set_verify, 3);
DEFINE_FUNC(SSL_CTX_use_PrivateKey, 2);

DEFINE_FUNC(SSL_CTX_use_PrivateKey_file, 3);
DEFINE_FUNC(SSL_CTX_use_certificate_chain_file, 2);

DEFINE_FUNC(ERR_get_error, 0);

static char ErrorString[] = "Ssl not found";
DEFINE_FUNC(ERR_error_string, 2, ErrorString);

// TLS functions

DEFINE_FUNC(TLS_client_method, 0);
DEFINE_FUNC(TLS_server_method, 0);

// RAND functions

DEFINE_FUNC(RAND_bytes, 2);

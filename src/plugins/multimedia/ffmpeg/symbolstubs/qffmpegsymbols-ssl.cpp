// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qsymbolsresolveutils_p.h>

#include <qstringliteral.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>

using namespace Qt::StringLiterals;

[[maybe_unused]] static constexpr auto SHLIB_VERSION =
#if defined(OPENSSL_SHLIB_VERSION)
    OPENSSL_SHLIB_VERSION;
#elif defined(SHLIB_VERSION_NUMBER)
    SHLIB_VERSION_NUMBER;
#endif


#if !defined(Q_OS_ANDROID)
CHECK_VERSIONS("ssl", SSL_NEEDED_SOVERSION, SHLIB_VERSION);
#endif

static std::unique_ptr<QLibrary> loadLib()
{
    auto lib = std::make_unique<QLibrary>();

    auto tryLoad = [&](QString sslName, auto version) {
        lib->setFileNameAndVersion(sslName, version);
        return lib->load();
    };

// openssl on Android has specific suffixes
#if defined(Q_OS_ANDROID)
    {
        auto suffix = qEnvironmentVariable("ANDROID_OPENSSL_SUFFIX");
        if (suffix.isEmpty()) {
#if (OPENSSL_VERSION_NUMBER >> 28) < 3 // major version < 3
            suffix = "_1_1"_L1;
#elif OPENSSL_VERSION_MAJOR == 3
            suffix = "_3"_L1;
#else
            static_assert(false, "Unexpected openssl version");
#endif
        }

        if (tryLoad("ssl"_L1 + suffix, -1))
            return lib;
    }
#endif

    if (tryLoad("ssl"_L1, SSL_NEEDED_SOVERSION ""_L1))
        return lib;

    return {};
};


BEGIN_INIT_FUNCS("ssl", loadLib)

// BN functions

INIT_FUNC(BN_value_one);
INIT_FUNC(BN_mod_word);

INIT_FUNC(BN_div_word)
INIT_FUNC(BN_mul_word)
INIT_FUNC(BN_add_word)
INIT_FUNC(BN_sub_word)
INIT_FUNC(BN_set_word)
INIT_FUNC(BN_new)
INIT_FUNC(BN_cmp)

INIT_FUNC(BN_free);

INIT_FUNC(BN_copy);

INIT_FUNC(BN_CTX_new);

INIT_FUNC(BN_CTX_free);
INIT_FUNC(BN_CTX_start);

INIT_FUNC(BN_CTX_get);
INIT_FUNC(BN_CTX_end);

INIT_FUNC(BN_rand);
INIT_FUNC(BN_mod_exp);

INIT_FUNC(BN_num_bits);
INIT_FUNC(BN_num_bits_word);

INIT_FUNC(BN_bn2hex);
INIT_FUNC(BN_bn2dec);

INIT_FUNC(BN_hex2bn);
INIT_FUNC(BN_dec2bn);
INIT_FUNC(BN_asc2bn);

INIT_FUNC(BN_bn2bin);
INIT_FUNC(BN_bin2bn);

// BIO-related functions

INIT_FUNC(BIO_new);
INIT_FUNC(BIO_free);

INIT_FUNC(BIO_read);
INIT_FUNC(BIO_write);
INIT_FUNC(BIO_s_mem);

INIT_FUNC(BIO_set_data);

INIT_FUNC(BIO_get_data);
INIT_FUNC(BIO_set_init);

INIT_FUNC(BIO_set_flags);
INIT_FUNC(BIO_test_flags);
INIT_FUNC(BIO_clear_flags);

INIT_FUNC(BIO_meth_new);
INIT_FUNC(BIO_meth_free);

INIT_FUNC(BIO_meth_set_write);
INIT_FUNC(BIO_meth_set_read);
INIT_FUNC(BIO_meth_set_puts);
INIT_FUNC(BIO_meth_set_gets);
INIT_FUNC(BIO_meth_set_ctrl);
INIT_FUNC(BIO_meth_set_create);
INIT_FUNC(BIO_meth_set_destroy);
INIT_FUNC(BIO_meth_set_callback_ctrl);

// SSL functions

INIT_FUNC(SSL_CTX_new);
INIT_FUNC(SSL_CTX_up_ref);
INIT_FUNC(SSL_CTX_free);

INIT_FUNC(SSL_new);
INIT_FUNC(SSL_up_ref);
INIT_FUNC(SSL_free);

INIT_FUNC(SSL_accept);
INIT_FUNC(SSL_stateless);
INIT_FUNC(SSL_connect);
INIT_FUNC(SSL_read);
INIT_FUNC(SSL_peek);
INIT_FUNC(SSL_write);
INIT_FUNC(SSL_ctrl);
INIT_FUNC(SSL_shutdown);
INIT_FUNC(SSL_set_bio);

// options are unsigned long in openssl 1.1.1, and uint64 in 3.x.x
INIT_FUNC(SSL_CTX_set_options);

INIT_FUNC(SSL_get_error);
INIT_FUNC(SSL_CTX_load_verify_locations);

INIT_FUNC(SSL_CTX_set_verify);
INIT_FUNC(SSL_CTX_use_PrivateKey);

INIT_FUNC(SSL_CTX_use_PrivateKey_file);
INIT_FUNC(SSL_CTX_use_certificate_chain_file);

INIT_FUNC(ERR_get_error);

INIT_FUNC(ERR_error_string);

// TLS functions

INIT_FUNC(TLS_client_method);
INIT_FUNC(TLS_server_method);

// RAND functions

INIT_FUNC(RAND_bytes);

END_INIT_FUNCS()

//////////// Define

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

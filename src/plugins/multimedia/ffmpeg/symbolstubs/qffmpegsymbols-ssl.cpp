// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:execute-external-code

#include <QtMultimedia/private/qsymbolsresolveutils_p.h>

#include <qstringliteral.h>

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/params.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

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

// ASN1 functions

INIT_FUNC(ASN1_INTEGER_set);
INIT_FUNC(ASN1_INTEGER_set_uint64);

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
INIT_FUNC(BIO_new_mem_buf);
INIT_FUNC(BIO_free);

INIT_FUNC(BIO_read);
INIT_FUNC(BIO_write);
INIT_FUNC(BIO_s_mem);
INIT_FUNC(BIO_ctrl);

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

INIT_FUNC(BIO_read_ex);

// DTLS functions
INIT_FUNC(DTLS_client_method);
INIT_FUNC(DTLS_get_data_mtu);
INIT_FUNC(DTLS_server_method);

// EVP functions

INIT_FUNC(EVP_PKEY_CTX_free);
INIT_FUNC(EVP_PKEY_CTX_new_from_name);
INIT_FUNC(EVP_PKEY_CTX_set_params);
INIT_FUNC(EVP_PKEY_Q_keygen);
INIT_FUNC(EVP_PKEY_keygen_init);
INIT_FUNC(EVP_PKEY_keygen);
INIT_FUNC(EVP_PKEY_free);
INIT_FUNC(EVP_PKEY_generate);
INIT_FUNC(EVP_sha1);
INIT_FUNC(EVP_sha256);

// PEM functions

INIT_FUNC(PEM_read_bio_PrivateKey);
INIT_FUNC(PEM_read_bio_X509);
INIT_FUNC(PEM_write_bio_PrivateKey);
INIT_FUNC(PEM_write_bio_X509);

// SSL functions

INIT_FUNC(SSL_CTX_ctrl);
INIT_FUNC(SSL_CTX_free);
INIT_FUNC(SSL_CTX_load_verify_locations);
INIT_FUNC(SSL_CTX_new);
INIT_FUNC(SSL_CTX_set_default_verify_paths);
INIT_FUNC(SSL_CTX_set_info_callback);
INIT_FUNC(SSL_CTX_set_options);
INIT_FUNC(SSL_CTX_set_tlsext_use_srtp);
INIT_FUNC(SSL_CTX_set_verify);
INIT_FUNC(SSL_CTX_up_ref);
INIT_FUNC(SSL_CTX_use_PrivateKey);
INIT_FUNC(SSL_CTX_use_PrivateKey_file);
INIT_FUNC(SSL_CTX_use_certificate)
INIT_FUNC(SSL_CTX_use_certificate_chain_file);

INIT_FUNC(SSL_new);
INIT_FUNC(SSL_up_ref);
INIT_FUNC(SSL_free);

INIT_FUNC(SSL_accept);
INIT_FUNC(SSL_do_handshake);
INIT_FUNC(SSL_stateless);
INIT_FUNC(SSL_connect);
INIT_FUNC(SSL_is_init_finished);
INIT_FUNC(SSL_read);
INIT_FUNC(SSL_peek);
INIT_FUNC(SSL_write);
INIT_FUNC(SSL_ctrl);
INIT_FUNC(SSL_get_ex_data);
INIT_FUNC(SSL_set1_host);
INIT_FUNC(SSL_set_accept_state);
INIT_FUNC(SSL_set_bio);
INIT_FUNC(SSL_set_connect_state);
INIT_FUNC(SSL_set_ex_data);
INIT_FUNC(SSL_set_hostflags);
INIT_FUNC(SSL_set_options);
INIT_FUNC(SSL_shutdown);
INIT_FUNC(SSL_state_string);
INIT_FUNC(SSL_state_string_long);

INIT_FUNC(SSL_export_keying_material);

// options are unsigned long in openssl 1.1.1, and uint64 in 3.x.x

INIT_FUNC(SSL_get_error);

INIT_FUNC(ERR_clear_error);
INIT_FUNC(ERR_get_error);
INIT_FUNC(ERR_error_string);
INIT_FUNC(ERR_error_string_n);

// TLS functions

INIT_FUNC(TLS_client_method);
INIT_FUNC(TLS_server_method);

// RAND functions

INIT_FUNC(RAND_bytes);

// X509 functions

INIT_FUNC(X509_NAME_add_entry_by_txt);
INIT_FUNC(X509_NAME_free);
INIT_FUNC(X509_NAME_new);
INIT_FUNC(X509_digest);
INIT_FUNC(X509_free);
INIT_FUNC(X509_get_serialNumber);
INIT_FUNC(X509_getm_notAfter);
INIT_FUNC(X509_getm_notBefore);
INIT_FUNC(X509_gmtime_adj);
INIT_FUNC(X509_new);
INIT_FUNC(X509_set_issuer_name);
INIT_FUNC(X509_set_pubkey);
INIT_FUNC(X509_set_subject_name);
INIT_FUNC(X509_set_version);
INIT_FUNC(X509_sign);

END_INIT_FUNCS()

//////////// Define

// ASN1 functions

DEFINE_FUNC(ASN1_INTEGER_set, 2);
DEFINE_FUNC(ASN1_INTEGER_set_uint64, 2);

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
DEFINE_FUNC(BIO_new_mem_buf, 2);
DEFINE_FUNC(BIO_free, 1);

DEFINE_FUNC(BIO_read, 3, -1);
DEFINE_FUNC(BIO_write, 3, -1);
DEFINE_FUNC(BIO_s_mem, 0);
DEFINE_FUNC(BIO_ctrl, 4, -1);

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

DEFINE_FUNC(BIO_read_ex, 4, -2);

// DTLS functions

DEFINE_FUNC(DTLS_client_method, 0);
DEFINE_FUNC(DTLS_get_data_mtu, 1);
DEFINE_FUNC(DTLS_server_method, 0);

// EVP functions

extern "C" [[maybe_unused]] EXPORT_FUNC EVP_PKEY *EVP_PKEY_Q_keygen(
    OSSL_LIB_CTX *libctx,
    const char *propq,
    const char *type,
    ...)
{
    const auto f = reinterpret_cast<decltype(::EVP_PKEY_Q_keygen)*>(
        SymbolsResolverImpl::instance().EVP_PKEY_Q_keygen);
    if (!f)
        return nullptr;

    va_list args;
    va_start(args, type);
    auto cleanup = qScopeGuard([&] {
        va_end(args);
    });

    if (strcmp(type, "RSA") == 0) {
        return f(libctx, propq, type, va_arg(args, size_t));
    } else if (strcmp(type, "EC") == 0) {
        return f(libctx, propq, type, va_arg(args, char *));
    }

    return f(libctx, propq, type);
}

DEFINE_FUNC(EVP_PKEY_CTX_free, 1);
DEFINE_FUNC(EVP_PKEY_CTX_new_from_name, 3);
DEFINE_FUNC(EVP_PKEY_CTX_set_params, 2);
DEFINE_FUNC(EVP_PKEY_free, 1);
DEFINE_FUNC(EVP_PKEY_generate, 2);
DEFINE_FUNC(EVP_PKEY_keygen_init, 1);
DEFINE_FUNC(EVP_PKEY_keygen, 2);
DEFINE_FUNC(EVP_sha1, 0);
DEFINE_FUNC(EVP_sha256, 0);

// PEM functions

DEFINE_FUNC(PEM_read_bio_PrivateKey, 4);
DEFINE_FUNC(PEM_read_bio_X509, 4);
DEFINE_FUNC(PEM_write_bio_PrivateKey, 7, -1);
DEFINE_FUNC(PEM_write_bio_X509, 2);

// SSL functions

DEFINE_FUNC(SSL_CTX_ctrl, 4, -1);
DEFINE_FUNC(SSL_CTX_free, 1);
DEFINE_FUNC(SSL_CTX_load_verify_locations, 3, -1);
DEFINE_FUNC(SSL_CTX_new, 1);
DEFINE_FUNC(SSL_CTX_set_default_verify_paths, 1);
DEFINE_FUNC(SSL_CTX_set_info_callback, 2);
DEFINE_FUNC(SSL_CTX_set_options, 2);
DEFINE_FUNC(SSL_CTX_set_tlsext_use_srtp, 2, 1);
DEFINE_FUNC(SSL_CTX_set_verify, 3);
DEFINE_FUNC(SSL_CTX_up_ref, 1);
DEFINE_FUNC(SSL_CTX_use_PrivateKey, 2);
DEFINE_FUNC(SSL_CTX_use_PrivateKey_file, 3);
DEFINE_FUNC(SSL_CTX_use_certificate, 2);
DEFINE_FUNC(SSL_CTX_use_certificate_chain_file, 2);

DEFINE_FUNC(SSL_new, 1);
DEFINE_FUNC(SSL_up_ref, 1);
DEFINE_FUNC(SSL_free, 1);

DEFINE_FUNC(SSL_accept, 1);
DEFINE_FUNC(SSL_do_handshake, 1, -1);
DEFINE_FUNC(SSL_stateless, 1);
DEFINE_FUNC(SSL_connect, 1);
DEFINE_FUNC(SSL_is_init_finished, 1, 0);
DEFINE_FUNC(SSL_read, 3, -1);
DEFINE_FUNC(SSL_peek, 3);
DEFINE_FUNC(SSL_write, 3, -1);
DEFINE_FUNC(SSL_ctrl, 4);
DEFINE_FUNC(SSL_get_ex_data, 2);
DEFINE_FUNC(SSL_set1_host, 2);
DEFINE_FUNC(SSL_set_accept_state, 1);
DEFINE_FUNC(SSL_set_bio, 3);
DEFINE_FUNC(SSL_set_connect_state, 1);
DEFINE_FUNC(SSL_set_ex_data, 3);
DEFINE_FUNC(SSL_set_hostflags, 2);
DEFINE_FUNC(SSL_set_options, 2);
DEFINE_FUNC(SSL_shutdown, 1);
DEFINE_FUNC(SSL_state_string, 1);
DEFINE_FUNC(SSL_state_string_long, 1);

DEFINE_FUNC(SSL_export_keying_material, 8, -1);

// options are unsigned long in openssl 1.1.1, and uint64 in 3.x.x

DEFINE_FUNC(SSL_get_error, 2);

DEFINE_FUNC(ERR_clear_error, 0);
DEFINE_FUNC(ERR_get_error, 0);
static char ErrorString[] = "Ssl not found";
DEFINE_FUNC(ERR_error_string, 2, ErrorString);
// TODO: We could implement this one when SSL is not linked.
DEFINE_FUNC(ERR_error_string_n, 3);

// TLS functions

DEFINE_FUNC(TLS_client_method, 0);
DEFINE_FUNC(TLS_server_method, 0);

// RAND functions

DEFINE_FUNC(RAND_bytes, 2);

// X509 functions
DEFINE_FUNC(X509_NAME_add_entry_by_txt, 7);
DEFINE_FUNC(X509_NAME_free, 1);
DEFINE_FUNC(X509_NAME_new, 0);
DEFINE_FUNC(X509_digest, 4, 0);
DEFINE_FUNC(X509_free, 1);
DEFINE_FUNC(X509_get_serialNumber, 1);
DEFINE_FUNC(X509_getm_notAfter, 1);
DEFINE_FUNC(X509_getm_notBefore, 1);
DEFINE_FUNC(X509_gmtime_adj, 2);
DEFINE_FUNC(X509_new, 0);
DEFINE_FUNC(X509_set_issuer_name, 2);
DEFINE_FUNC(X509_set_pubkey, 2);
DEFINE_FUNC(X509_set_subject_name, 2);
DEFINE_FUNC(X509_set_version, 2);
DEFINE_FUNC(X509_sign, 3);

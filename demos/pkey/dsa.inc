/*-
 * Copyright 2022 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/pem.h>
#include <openssl/core_names.h>

/* Raw DSA params for P, Q and G */
static const unsigned char dsa_p[] = {
    0xa2, 0x9b, 0x88, 0x72, 0xce, 0x8b, 0x84, 0x23,
    0xb7, 0xd5, 0xd2, 0x1d, 0x4b, 0x02, 0xf5, 0x7e,
    0x03, 0xe9, 0xe6, 0xb8, 0xa2, 0x58, 0xdc, 0x16,
    0x61, 0x1b, 0xa0, 0x98, 0xab, 0x54, 0x34, 0x15,
    0xe4, 0x15, 0xf1, 0x56, 0x99, 0x7a, 0x3e, 0xe2,
    0x36, 0x65, 0x8f, 0xa0, 0x93, 0x26, 0x0d, 0xe3,
    0xad, 0x42, 0x2e, 0x05, 0xe0, 0x46, 0xf9, 0xec,
    0x29, 0x16, 0x1a, 0x37, 0x5f, 0x0e, 0xb4, 0xef,
    0xfc, 0xef, 0x58, 0x28, 0x5c, 0x5d, 0x39, 0xed,
    0x42, 0x5d, 0x7a, 0x62, 0xca, 0x12, 0x89, 0x6c,
    0x4a, 0x92, 0xcb, 0x19, 0x46, 0xf2, 0x95, 0x2a,
    0x48, 0x13, 0x3f, 0x07, 0xda, 0x36, 0x4d, 0x1b,
    0xdf, 0x6b, 0x0f, 0x71, 0x39, 0x98, 0x3e, 0x69,
    0x3c, 0x80, 0x05, 0x9b, 0x0e, 0xac, 0xd1, 0x47,
    0x9b, 0xa9, 0xf2, 0x85, 0x77, 0x54, 0xed, 0xe7,
    0x5f, 0x11, 0x2b, 0x07, 0xeb, 0xbf, 0x35, 0x34,
    0x8b, 0xbf, 0x3e, 0x01, 0xe0, 0x2f, 0x2d, 0x47,
    0x3d, 0xe3, 0x94, 0x53, 0xf9, 0x9d, 0xd2, 0x36,
    0x75, 0x41, 0xca, 0xca, 0x3b, 0xa0, 0x11, 0x66,
    0x34, 0x3d, 0x7b, 0x5b, 0x58, 0xa3, 0x7b, 0xd1,
    0xb7, 0x52, 0x1d, 0xb2, 0xf1, 0x3b, 0x86, 0x70,
    0x71, 0x32, 0xfe, 0x09, 0xf4, 0xcd, 0x09, 0xdc,
    0x16, 0x18, 0xfa, 0x34, 0x01, 0xeb, 0xf9, 0xcc,
    0x7b, 0x19, 0xfa, 0x94, 0xaa, 0x47, 0x20, 0x88,
    0x13, 0x3d, 0x6c, 0xb2, 0xd3, 0x5c, 0x11, 0x79,
    0xc8, 0xc8, 0xff, 0x36, 0x87, 0x58, 0xd5, 0x07,
    0xd9, 0xf9, 0xa1, 0x7d, 0x46, 0xc1, 0x10, 0xfe,
    0x31, 0x44, 0xce, 0x9b, 0x02, 0x2b, 0x42, 0xe4,
    0x19, 0xeb, 0x4f, 0x53, 0x88, 0x61, 0x3b, 0xfc,
    0x3e, 0x26, 0x24, 0x1a, 0x43, 0x2e, 0x87, 0x06,
    0xbc, 0x58, 0xef, 0x76, 0x11, 0x72, 0x78, 0xde,
    0xab, 0x6c, 0xf6, 0x92, 0x61, 0x82, 0x91, 0xb7
};

static const unsigned char dsa_q[] = {
    0xa3, 0xbf, 0xd9, 0xab, 0x78, 0x84, 0x79, 0x4e,
    0x38, 0x34, 0x50, 0xd5, 0x89, 0x1d, 0xc1, 0x8b,
    0x65, 0x15, 0x7b, 0xdc, 0xfc, 0xda, 0xc5, 0x15,
    0x18, 0x90, 0x28, 0x67
};

static const unsigned char dsa_g[] = {
    0x68, 0x19, 0x27, 0x88, 0x69, 0xc7, 0xfd, 0x3d,
    0x2d, 0x7b, 0x77, 0xf7, 0x7e, 0x81, 0x50, 0xd9,
    0xad, 0x43, 0x3b, 0xea, 0x3b, 0xa8, 0x5e, 0xfc,
    0x80, 0x41, 0x5a, 0xa3, 0x54, 0x5f, 0x78, 0xf7,
    0x22, 0x96, 0xf0, 0x6c, 0xb1, 0x9c, 0xed, 0xa0,
    0x6c, 0x94, 0xb0, 0x55, 0x1c, 0xfe, 0x6e, 0x6f,
    0x86, 0x3e, 0x31, 0xd1, 0xde, 0x6e, 0xed, 0x7d,
    0xab, 0x8b, 0x0c, 0x9d, 0xf2, 0x31, 0xe0, 0x84,
    0x34, 0xd1, 0x18, 0x4f, 0x91, 0xd0, 0x33, 0x69,
    0x6b, 0xb3, 0x82, 0xf8, 0x45, 0x5e, 0x98, 0x88,
    0xf5, 0xd3, 0x1d, 0x47, 0x84, 0xec, 0x40, 0x12,
    0x02, 0x46, 0xf4, 0xbe, 0xa6, 0x17, 0x94, 0xbb,
    0xa5, 0x86, 0x6f, 0x09, 0x74, 0x64, 0x63, 0xbd,
    0xf8, 0xe9, 0xe1, 0x08, 0xcd, 0x95, 0x29, 0xc3,
    0xd0, 0xf6, 0xdf, 0x80, 0x31, 0x6e, 0x2e, 0x70,
    0xaa, 0xeb, 0x1b, 0x26, 0xcd, 0xb8, 0xad, 0x97,
    0xbc, 0x3d, 0x28, 0x7e, 0x0b, 0x8d, 0x61, 0x6c,
    0x42, 0xe6, 0x5b, 0x87, 0xdb, 0x20, 0xde, 0xb7,
    0x00, 0x5b, 0xc4, 0x16, 0x74, 0x7a, 0x64, 0x70,
    0x14, 0x7a, 0x68, 0xa7, 0x82, 0x03, 0x88, 0xeb,
    0xf4, 0x4d, 0x52, 0xe0, 0x62, 0x8a, 0xf9, 0xcf,
    0x1b, 0x71, 0x66, 0xd0, 0x34, 0x65, 0xf3, 0x5a,
    0xcc, 0x31, 0xb6, 0x11, 0x0c, 0x43, 0xda, 0xbc,
    0x7c, 0x5d, 0x59, 0x1e, 0x67, 0x1e, 0xaf, 0x7c,
    0x25, 0x2c, 0x1c, 0x14, 0x53, 0x36, 0xa1, 0xa4,
    0xdd, 0xf1, 0x32, 0x44, 0xd5, 0x5e, 0x83, 0x56,
    0x80, 0xca, 0xb2, 0x53, 0x3b, 0x82, 0xdf, 0x2e,
    0xfe, 0x55, 0xec, 0x18, 0xc1, 0xe6, 0xcd, 0x00,
    0x7b, 0xb0, 0x89, 0x75, 0x8b, 0xb1, 0x7c, 0x2c,
    0xbe, 0x14, 0x44, 0x1b, 0xd0, 0x93, 0xae, 0x66,
    0xe5, 0x97, 0x6d, 0x53, 0x73, 0x3f, 0x4f, 0xa3,
    0x26, 0x97, 0x01, 0xd3, 0x1d, 0x23, 0xd4, 0x67
};

/* Helper function to retrieve and print a key BIGNUM field */
static void print_bn(BIO *bio, const EVP_PKEY *pkey, const char *name)
{
    BIGNUM *bn = NULL;

    if (EVP_PKEY_get_bn_param(pkey, name, &bn) == 0)
        return;

    BIO_printf(bio, "  %s = 0x", name);
    BN_print(bio, bn);
    BIO_printf(bio, "\n");
    BN_clear_free(bn);
}

/*
 * Print DSA key info
 *
 * This method shows how to extract DSA data from an EVP_PKEY.
 * There are simpler ways to print using EVP_PKEY_print_XXXX().
 */
static int dsa_print_key(const EVP_PKEY *pkey, int keypair,
                         OSSL_LIB_CTX *libctx, const char *propq)
{

    int rv = 0, gindex, counter;
    BIO *bio = BIO_new_fp(stdout, BIO_NOCLOSE);
    unsigned char seed[2048];
    size_t seedlen;

    if (bio == NULL)
        return 0;

    /* Output hexadecimal representations of the BIGNUM objects. */
    BIO_printf(bio, "\nPublic values:\n");
    print_bn(bio, pkey, OSSL_PKEY_PARAM_FFC_P);
    print_bn(bio, pkey, OSSL_PKEY_PARAM_FFC_Q);
    print_bn(bio, pkey, OSSL_PKEY_PARAM_FFC_G);

    if (EVP_PKEY_get_octet_string_param(pkey, OSSL_PKEY_PARAM_FFC_SEED, seed,
                                        sizeof(seed), &seedlen) > 0) {
        BIO_printf(bio, "  %s\n", OSSL_PKEY_PARAM_FFC_SEED);
        BIO_dump(bio, seed, seedlen);
    }
    if (EVP_PKEY_get_int_param(pkey, OSSL_PKEY_PARAM_FFC_GINDEX, &gindex) > 0) {
        if (gindex != -1)
            BIO_printf(bio, "  %s = %d\n", OSSL_PKEY_PARAM_FFC_GINDEX, gindex);
    }
    if (EVP_PKEY_get_int_param(pkey, OSSL_PKEY_PARAM_FFC_PCOUNTER, &counter) > 0) {
        if (counter != -1)
            BIO_printf(bio, "  %s = %d\n", OSSL_PKEY_PARAM_FFC_PCOUNTER, counter);
    }

    if (keypair) {
        fprintf(stdout, "\nPrivate value:\n");
        print_bn(bio, pkey, OSSL_PKEY_PARAM_PRIV_KEY);

        /* Output a PEM encoding of the public key. */
        if (PEM_write_bio_PUBKEY_ex(bio, pkey, libctx, propq) == 0) {
            fprintf(stderr, "Failed to output PEM-encoded public key\n");
            goto cleanup;
        }

        /*
         * Output a PEM encoding of the private key. Please note that this output is
         * not encrypted. You may wish to use the arguments to specify encryption of
         * the key if you are storing it on disk. See PEM_write_bio_PrivateKey_ex(3).
         */
        if (PEM_write_bio_PrivateKey_ex(bio, pkey, NULL, NULL, 0, NULL, NULL, libctx, propq) == 0) {
            fprintf(stderr, "Failed to output PEM-encoded private key\n");
            goto cleanup;
        }
    } else {
        if (PEM_write_bio_Parameters(bio, pkey) == 0) {
            fprintf(stderr, "Failed to output PEM-encoded params\n");
            goto cleanup;
        }
    }
    rv = 1;
cleanup:
    BIO_free(bio);
    return rv;
}
// SPDX-License-Identifier: MIT
/*
 * MSS signature verification demonstration for the Microchip PolarFire SoC
 *
 * Copyright (c) 2021 Microchip Technology Inc. All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <openssl/err.h>
#include <openssl/ecdsa.h>
#include <openssl/pem.h>

#define BUFFSIZE 4096
#define ALGO "SHA384"
#define SIGN_SIZE 104
#define SIGN_READ_BYTES (3 + 2 * SIGN_SIZE) //3 for status and a space, 192 chars for the hex signature (96 bytes)
#define SIGN_DEVICE "/dev/mpfs_signature"
#define CERT_SIZE 1024
#define CERT_READ_BYTES (3 + 2 * CERT_SIZE) //3 for status and a space, 2048 chars for the hex signature (96 bytes)
#define CERT_DEVICE "/dev/mpfs_device_cert_num"

void get_hash(const unsigned char *msg, const size_t msg_len, unsigned char *hash, size_t *hash_len)
{
    EVP_MD_CTX *hashctx;
    hashctx = EVP_MD_CTX_new();
    const EVP_MD *hashptr = EVP_get_digestbyname(ALGO);

    /* setup hashing environment */
    EVP_MD_CTX_init(hashctx);
    EVP_DigestInit_ex(hashctx, hashptr, NULL);
    /* add message to string to be hashed */
    EVP_DigestUpdate(hashctx, msg, msg_len);
    /* acquire hash */
    EVP_DigestFinal_ex(hashctx, hash, (unsigned int *)hash_len);
    /* perform cleanup */
    EVP_MD_CTX_free(hashctx);
}

void get_signature(const unsigned char *hash, const size_t hash_len, const size_t outbufflen, unsigned char *outbuff)
{
    unsigned char inbuff[BUFFSIZE];
    unsigned char *buff = inbuff + 3; // + 3 to skip response code
    int inc;
    FILE *fptr;

    if ((fptr = fopen(SIGN_DEVICE, "w")) == NULL)
    {
        printf("Error! opening file");
        exit(1);
    }
    /* write 38 byte hash to the device file */
    for (inc = 0; inc < hash_len; ++inc)
        fprintf(fptr, "%c", hash[inc]);
    fclose(fptr);
    if ((fptr = fopen(SIGN_DEVICE, "r")) == NULL)
    {
        printf("Error! opening signature device file ile");
        exit(1);
    }
    /* read back 104 byte DER format signature */
    fread(inbuff, SIGN_READ_BYTES, 1, fptr);
    fclose(fptr);

    /* convert hex string to char */
    for (size_t count = 0; count < outbufflen; count++)
    {
        sscanf(buff, "%2hhx", &outbuff[count]);
        buff += 2;
    }
}

void get_cert(const size_t outbufflen, unsigned char *outbuff)
{
    unsigned char inbuff[BUFFSIZE];
    unsigned char *buff = inbuff + 3; // + 3 to skip response code
    FILE *fptr;

    if ((fptr = fopen(CERT_DEVICE, "r")) == NULL)
    {
        printf("Error! opening certificate device file");
        exit(1);
    }
    /* read 1024 byte cert from device file */
    fread(inbuff, CERT_READ_BYTES, 1, fptr);
    fclose(fptr);

    /* convert hex string to char */
    for (size_t count = 0; count < outbufflen; count++)
    {
        sscanf(buff, "%2hhx", &outbuff[count]);
        buff += 2;
    }
}

int main()
{
    unsigned char hash[BUFFSIZE] = "123";
    unsigned char msg[BUFFSIZE] = "signature-verification-demo";
    unsigned char sign_raw[SIGN_SIZE];
    unsigned char cert_raw[CERT_SIZE];
    size_t msg_len = strlen((const char *)msg), hash_len = 0;
    unsigned char *ptr;
    int inc;

    /* required openssl objects */
    ECDSA_SIG *sig = ECDSA_SIG_new();
    X509 *cert = X509_new();
    EVP_PKEY *pubkey;
    EC_KEY *ec_key;
    BIO *out = BIO_new_fp(stdout, BIO_NOCLOSE);

    /* prepare openssl for use */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    ERR_load_BIO_strings();

    /* get the message hashed using via openssl */
    get_hash(msg, msg_len, hash, &hash_len);

    printf("\r\nmsg: %s\r\nhash: ", msg);
    for (inc = 0; inc < hash_len; ++inc)
        printf("%02x", hash[inc]);
    printf(" \r\nhash length: %zu\r\n", hash_len);

    /* use the digital signature service to sign the 48 byte hash */
    get_signature(hash, hash_len, SIGN_SIZE, sign_raw);

    printf("sig: ");
    for (inc = 0; inc < SIGN_SIZE; inc++)
        printf("%02x", sign_raw[inc]);
    printf("\r\n");

    /* convert raw signature to openssl ecdsa object */
    ptr = sign_raw;
    sig = d2i_ECDSA_SIG(&sig, (const unsigned char **)&ptr, SIGN_SIZE);
    if (sig == NULL)
    {
        printf("error: message signature is invalid\r\n");
        exit(1);
    }

    /* print both components of the signature */
    printf("(sig->r, sig->s): (%s,%s)\n", BN_bn2hex(ECDSA_SIG_get0_r(sig)), BN_bn2hex(ECDSA_SIG_get0_s(sig)));

    /* get device supply chain assurance certificate from the device certificate service*/
    get_cert(CERT_SIZE, cert_raw);

    /* convert and parse the raw signature as an x509 object using openssl */
    ptr = cert_raw;
    cert = d2i_X509(&cert, (const unsigned char **)&ptr, CERT_SIZE);
    if (cert == NULL)
    {
        printf("error: device certificate not found\r\n");
        exit(1);
    }

    /* get ecdsa public key from cert */
    pubkey = X509_get_pubkey(cert);
    if (pubkey == NULL) {
        printf("error: public key not found in certificate\r\n");
        exit(1);
    }

    PEM_write_bio_PUBKEY(out, pubkey);

    /* check signature against message hash using public key */
    ec_key = EVP_PKEY_get1_EC_KEY(pubkey);

    /* cleanup unneeded objects */
    X509_free(cert);
    EVP_PKEY_free(pubkey);

    /* finally verifiy the signature using the hash of the original message and the EC public key */
    int ret1 = ECDSA_do_verify(hash, hash_len, sig, ec_key);
    if (ret1 > 0)
    {
        printf("success: signature matches\r\n");
    }
    else if (ret1 < 0)
    {
        printf("error: something went wrong during verification\r\n");
        exit(1);
    }
    else
    {
        printf("failure: signature does not match\r\n");
    }

    /* cleanup unneeded objects */
    ECDSA_SIG_free(sig);
    EC_KEY_free(ec_key);

    return 0;
}
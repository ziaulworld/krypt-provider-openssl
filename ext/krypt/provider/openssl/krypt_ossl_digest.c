/*
* krypt-provider OpenSSL
*
* Copyright (C) 2012
* Hiroshi Nakamura <nahi@ruby-lang.org>
* Martin Bosslet <martin.bosslet@googlemail.com>
* All rights reserved.
*
* This software is distributed under the same license as Ruby.
* See the file 'LICENSE' for further details.
*/

#include "krypt-provider.h"
#include "krypt-provider-ossl.h"

typedef struct krypt_md_ossl_st {
    krypt_provider *provider;
    krypt_interface_md *methods;
    EVP_MD_CTX *ctx;
} krypt_md_ossl;

#define int_safe_cast(dest, md)					\
do {								\
    if ((md)->provider != &krypt_provider_ossl) return 0;	\
    (dest) = (krypt_md_ossl *) (md);				\
} while (0)

int int_md_reset(krypt_md *md);
int int_md_update(krypt_md *md, unsigned char *data, size_t len);
int int_md_final(krypt_md *md, unsigned char ** digest, size_t *len);
int int_md_digest(krypt_md *md, unsigned char *data, size_t len, unsigned char **digest, size_t *digest_len);
int int_md_length(krypt_md *md, int *len);
int int_md_block_length(krypt_md *md, int *len);
int int_md_name(krypt_md *md, const char **name);
void int_md_mark(krypt_md *md);
void int_md_free(krypt_md *md);

krypt_interface_md krypt_interface_md_ossl = {
    int_md_reset,
    int_md_update,
    int_md_final,
    int_md_digest,
    int_md_length,
    int_md_block_length,
    int_md_name,
    NULL,
    int_md_free
};

static krypt_md_ossl *
int_md_alloc(void)
{
    krypt_md_ossl *md = ALLOC(krypt_md_ossl);
    memset(md, 0, sizeof(krypt_md_ossl));
    md->provider = &krypt_provider_ossl;
    md->methods = &krypt_interface_md_ossl;
    return md;
}

#define return_krypt_ossl_md()					\
do {								\
    if (!md) return NULL;					\
    if (!(ctx = EVP_MD_CTX_create())) return NULL;		\
    if (!(ret = int_md_alloc())) return NULL;			\
    if (EVP_DigestInit_ex(ctx, md, NULL) != 1) return NULL;	\
    ret->ctx = ctx;						\
    return (krypt_md *)ret;					\
} while (0)

krypt_md *
krypt_ossl_md_new_oid(krypt_provider *provider, const char *oid)
{
    EVP_MD_CTX *ctx;
    const EVP_MD *md;
    krypt_md_ossl *ret;
    ASN1_OBJECT *ossl_oid;

    if (provider != &krypt_provider_ossl) return NULL;

    ossl_oid = OBJ_txt2obj(oid, 0);
    md = EVP_get_digestbyobj(ossl_oid);
    ASN1_OBJECT_free(ossl_oid);
    return_krypt_ossl_md();
}

krypt_md *
krypt_ossl_md_new_name(krypt_provider *provider, const char *name)
{
    EVP_MD_CTX *ctx;
    const EVP_MD *md;
    krypt_md_ossl *ret;

    if (provider != &krypt_provider_ossl) return NULL;

    md = EVP_get_digestbyname(name);
    return_krypt_ossl_md();
}

int
int_md_reset(krypt_md *ext)
{
    krypt_md_ossl *md;

    int_safe_cast(md, ext);
    if (EVP_DigestInit_ex(md->ctx, EVP_MD_CTX_md(md->ctx), NULL) != 1)
	return 0;
    return 1;
}

int
int_md_update(krypt_md *ext, unsigned char *data, size_t len)
{
    krypt_md_ossl *md;

    int_safe_cast(md, ext);
    EVP_DigestUpdate(md->ctx, data, len);
    return 1;
}

int
int_md_final(krypt_md *ext, unsigned char ** digest, size_t *len)
{
    krypt_md_ossl *md;
    unsigned char *ret;
    size_t ret_len;

    int_safe_cast(md, ext);
    ret_len = EVP_MD_CTX_size(md->ctx);
    ret = ALLOC_N(unsigned char, ret_len);
    EVP_DigestFinal_ex(md->ctx, ret, NULL);

    *digest = ret;
    *len = ret_len;
    return 1;
}

int
int_md_digest(krypt_md *ext, unsigned char *data, size_t len, unsigned char **digest, size_t *digest_len)
{
    if (!int_md_update(ext, data, len)) return 0;
    return int_md_final(ext, digest, digest_len);
}

int
int_md_length(krypt_md *ext, int *len)
{
    krypt_md_ossl *md;

    int_safe_cast(md, ext);
    *len = EVP_MD_CTX_size(md->ctx);
    return 1;
}

int
int_md_block_length(krypt_md *ext, int *len)
{
    krypt_md_ossl *md;

    int_safe_cast(md, ext);
    *len = EVP_MD_CTX_block_size(md->ctx);
    return 1;
}

int
int_md_name(krypt_md *ext, const char **name)
{
    krypt_md_ossl *md;

    int_safe_cast(md, ext);
    *name = EVP_MD_name(EVP_MD_CTX_md(md->ctx));
    return 1;
}

void
int_md_free(krypt_md *ext)
{
    krypt_md_ossl *md;

    if (!ext) return;
    if ((ext)->provider != &krypt_provider_ossl) return;
    md = (krypt_md_ossl *) ext;
    if (md->ctx)
	EVP_MD_CTX_destroy(md->ctx);
    xfree(md);
}

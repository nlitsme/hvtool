#ifndef __CRYPTO_HASH_H__
#define __CRYPTO_HASH_H__
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/ripemd.h>
#include <stdint.h>
#include <string.h>
#include <vector>

class hash {
public:
    virtual ~hash() { }
    virtual void add(const uint8_t *data, size_t size)= 0;
    virtual void final(uint8_t *hash)= 0;

    virtual size_t digestsize()= 0;
    virtual size_t blocksize()= 0;
};
#define declarehash(NAME, CTX, DIGESTSIZE, BLOCKSIZE, INIT, ADD, FINAL) \
class NAME : public hash { \
    CTX ctx; \
public:  \
    enum { DigestSize= DIGESTSIZE }; \
    enum { BlockSize= BLOCKSIZE }; \
    virtual size_t digestsize() { return DigestSize; } \
    virtual size_t blocksize() { return BlockSize; } \
    static const char*name() { return #NAME; } \
    NAME() \
    { \
        INIT(&ctx); \
    } \
    template<typename RANGE> \
    NAME(RANGE r) \
        : NAME() \
    { \
       ADD(&ctx, &r[0], r.size()); \
    } \
    NAME(const uint8_t *data, size_t size) \
        : NAME() \
    { \
       ADD(&ctx, data, size); \
    } \
    virtual void add(const uint8_t *data, size_t size) \
    { \
        ADD(&ctx, data, size); \
    } \
    virtual void final(uint8_t *hash) \
    { \
        FINAL(hash, &ctx); \
        memset(&ctx, 0, sizeof(ctx)); \
    } \
    virtual std::vector<uint8_t> digest() \
    { \
        std::vector<uint8_t> h(DigestSize); \
        FINAL(&h[0], &ctx); \
        return h; \
    } \
};

declarehash(Sha1, SHA_CTX, SHA_DIGEST_LENGTH, 64, SHA1_Init, SHA1_Update, SHA1_Final)
declarehash(Sha256, SHA256_CTX, SHA256_DIGEST_LENGTH, 64, SHA256_Init, SHA256_Update, SHA256_Final)
declarehash(Sha512, SHA512_CTX, SHA512_DIGEST_LENGTH, 128, SHA512_Init, SHA512_Update, SHA512_Final)
declarehash(Md5, MD5_CTX, MD5_DIGEST_LENGTH, 64, MD5_Init, MD5_Update, MD5_Final)
declarehash(Ripemd160, RIPEMD160_CTX, RIPEMD160_DIGEST_LENGTH, 64, RIPEMD160_Init, RIPEMD160_Update, RIPEMD160_Final)

#endif

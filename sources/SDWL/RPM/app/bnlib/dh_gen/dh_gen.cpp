

#include <windows.h>
#include <assert.h>


#include <openssl/bn.h>

#include "..\include\dh_gen.h"

bool dh_generate_y(_In_ const std::vector<unsigned char>& p, _In_ const std::vector<unsigned char>& g, _In_ const std::vector<unsigned char>& x, _Out_ std::vector<unsigned char>& y)
{
    bool result = false;
    BN_CTX* ctx = NULL;
    BIGNUM* bn_p = NULL;
    BIGNUM* bn_g = NULL;
    BIGNUM* bn_x = NULL;
    BIGNUM* bn_y = NULL;

    do {

        ctx = BN_CTX_new();
        if (NULL == ctx) {
            break;
        }

        BN_CTX_start(ctx);

        bn_p = BN_bin2bn(p.data(), (int)p.size(), NULL);
        if (NULL == bn_p) {
            break;
        }

        bn_g = BN_bin2bn(g.data(), (int)g.size(), NULL);
        if (NULL == bn_g) {
            break;
        }

        bn_x = BN_bin2bn(x.data(), (int)x.size(), NULL);
        if (NULL == bn_x) {
            break;
        }

        bn_y = BN_new();
        if (NULL == bn_y) {
            break;
        }

        if (BN_mod_exp(bn_y, bn_g, bn_x, bn_p, ctx)) {
            y.resize(x.size(), 0);
            const size_t y_size = BN_bn2bin(bn_y, y.data());
            assert(y_size == x.size());
            result = true;
        }

    } while (false);

    if (bn_p) {
        BN_free(bn_p);
    }

    if (bn_g) {
        BN_free(bn_g);
    }

    if (bn_x) {
        BN_free(bn_x);
    }

    if (bn_y) {
        BN_free(bn_y);
    }

    if (NULL != ctx) {
        BN_CTX_end(ctx);
        BN_CTX_free(ctx);
        ctx = NULL;
    }

    return result;
}
/* © 2015 Silicon Laboratories Inc.
 */
#include <gtest/gtest.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include <curve25519.h>
}

#define KEY_SIZE (32)

static const uint8_t alice_secret_key[KEY_SIZE] = {0x77, 0x07, 0x6d, 0x0a, 0x73, 0x18, 0xa5, 0x7d, 0x3c, 0x16, 0xc1, 0x72, 0x51, 0xb2, 0x66, 0x45, 0xdf, 0x4c, 0x2f, 0x87, 0xeb, 0xc0, 0x99, 0x2a, 0xb1, 0x77, 0xfb, 0xa5, 0x1d, 0xb9, 0x2c, 0x2a};

static void bigint_mul(unsigned char *r, const unsigned char *a, const unsigned char *b, uint8_t len)
{
    uint8_t i;
    uint8_t j;
    uint16_t t;
    for (i = 0; i < 2 * len; i++)
        r[i] = 0;

    for (i = 0; i < len; i++) {
        t = 0;
        for (j = 0; j < len; j++) {
            t        = r[i + j] + a[i] * b[j] + (t >> 8);
            r[i + j] = (t & 0xFF);
        }
        r[i + len] = (t >> 8);
    }
}

static uint8_t alice_public_key[KEY_SIZE];

TEST(Curve25519, AliceCalculationOfPublicKey)
{
    const uint8_t expected_alice_public_key[KEY_SIZE] = {0x85, 0x20, 0xf0, 0x09, 0x89, 0x30, 0xa7, 0x54, 0x74, 0x8b, 0x7d, 0xdc, 0xb4, 0x3e, 0xf7, 0x5a, 0x0d, 0xbf, 0x3a, 0x0d, 0x26, 0x38, 0x1a, 0xf4, 0xeb, 0xa4, 0xa9, 0x8e, 0xaa, 0x9b, 0x4e, 0x6a};

    crypto_scalarmult_curve25519_base(alice_public_key, alice_secret_key);

    EXPECT_EQ(0, memcmp(expected_alice_public_key, alice_public_key, KEY_SIZE));
}

TEST(Curve25519, BigintCalc)
{
    const uint8_t some_data1[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12};
    const uint8_t some_data2[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12};
    uint8_t result[16];

    memset(result, 0x00, sizeof(result));
    bigint_mul(result, some_data1, some_data2, 8);
    EXPECT_EQ(0x44u, result[14]);
    EXPECT_EQ(0x01u, result[15]);
}

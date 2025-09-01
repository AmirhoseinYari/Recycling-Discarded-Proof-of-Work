#include <Arduino.h>
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/rsa.h"
#include "mbedtls/sha256.h"

//–– Configuration ––
static const int   NUM_VER = 100;
static const char* RNG_PERS = "rsa_different_sizes";

struct Test { int bits; int exp; };
static const Test tests[] = {
  {1024, 65537},
  {2048, 65537},
  {3072, 65537},
  {4096, 65537}
};

//–– Global mbedTLS contexts ––
mbedtls_entropy_context  entropy;
mbedtls_ctr_drbg_context ctr_drbg;

void setup() {
  Serial.begin(115200);
  while(!Serial);    
  Serial.println("\n=== RSA-Verify: Different Sizes ===");

  // Seed the DRBG once
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  if (mbedtls_ctr_drbg_seed(&ctr_drbg,
        mbedtls_entropy_func, &entropy,
        (const unsigned char*)RNG_PERS, strlen(RNG_PERS)) != 0) {
    Serial.println("DRBG seed failed"); while(1);
  }

  // For each test case: generate key, sign once, then benchmark verify
  for (auto &tc : tests) {
    Serial.printf("\n-- RSA-%d, e=%u --\n", tc.bits, tc.exp);

    // 1) Init & generate
    mbedtls_rsa_context rsa;
    mbedtls_rsa_init(&rsa);
    uint32_t t0 = millis();
    if (mbedtls_rsa_gen_key(
          &rsa,
          mbedtls_ctr_drbg_random, &ctr_drbg,
          tc.bits, tc.exp) != 0) {
      Serial.println("  Keygen FAILED"); while(1);
    }
    Serial.printf("  Keygen: %u ms\n", millis() - t0);

    // 2) Hash a constant message
    const char* msg = "ESP32 RSA test";
    uint8_t hash[32];
    mbedtls_sha256((const uint8_t*)msg, strlen(msg), hash, 0);

    // 3) Sign once
    uint8_t sig[MBEDTLS_MPI_MAX_SIZE];
    t0 = millis();
    if (mbedtls_rsa_pkcs1_sign(
          &rsa,
          mbedtls_ctr_drbg_random, &ctr_drbg,
          MBEDTLS_MD_SHA256,
          sizeof(hash), hash,
          sig) != 0) {
      Serial.println("  Sign FAILED"); while(1);
    }
    Serial.printf("  Sign   : %u ms\n", millis() - t0);

    // 4) Benchmark NUM_VER verifies
    Serial.print("  Verifying ");
    Serial.print(NUM_VER);
    Serial.print("× … ");
    uint32_t vst = millis();
    for (int i = 0; i < NUM_VER; i++) {
      mbedtls_rsa_pkcs1_verify(
        &rsa,
        MBEDTLS_MD_SHA256,
        sizeof(hash), hash,
        sig
      );
    }
    uint32_t dt = millis() - vst;
    Serial.printf("total %u ms, avg %.2f ms\n",
                  dt, float(dt)/NUM_VER);

    mbedtls_rsa_free(&rsa);
  }

  // Cleanup
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);

  Serial.println("\n=== All Tests Complete ===");
}

void loop() {
  // nothing—benchmarks run once in setup()
}
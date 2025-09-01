#include <Arduino.h>
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/rsa.h"
#include "mbedtls/sha256.h"

#define NUM_RUNS 100
#define RSA_BITS 2048
#define RSA_EXP  65537

mbedtls_entropy_context  entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_rsa_context      rsa;

void setup() {
  Serial.begin(115200);
  while(!Serial);

  Serial.println("\n=== RSA-2048 Verify Benchmark (100 different sigs) ===");

  // 1) Seed RNG
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  if (mbedtls_ctr_drbg_seed(
        &ctr_drbg,
        mbedtls_entropy_func, &entropy,
        (const unsigned char*)"rsa_verify", strlen("rsa_verify")
      ) != 0) {
    Serial.println("DRBG seed failed");
    while(1);
  }

  // 2) Generate one 2048-bit keypair
  mbedtls_rsa_init(&rsa);
  Serial.print("Generating 2048-bit RSA key… ");
  uint32_t t0 = millis();
  if (mbedtls_rsa_gen_key(
        &rsa,
        mbedtls_ctr_drbg_random, &ctr_drbg,
        RSA_BITS, RSA_EXP
      ) != 0) {
    Serial.println("KEYGEN FAIL");
    while(1);
  }
  Serial.printf("done in %u ms\n\n", millis() - t0);

  // buffers for each run
  uint8_t hash[32];
  uint8_t sig[MBEDTLS_MPI_MAX_SIZE];
  size_t  sig_len = 0;

  // Accumulate total verify time
  uint32_t total_verify_ms = 0;

  for (int i = 0; i < NUM_RUNS; i++) {
    // prepare a unique message
    char msg[32];
    int  msg_len = snprintf(msg, sizeof(msg), "Message #%d", i);

    // hash it
    mbedtls_sha256((const uint8_t*)msg, msg_len, hash, 0);

    // sign it (we don't time this)
    if (mbedtls_rsa_pkcs1_sign(
          &rsa,
          mbedtls_ctr_drbg_random, &ctr_drbg,
          MBEDTLS_MD_SHA256,
          sizeof(hash), hash,
          sig
        ) != 0) {
      Serial.printf("Sign failed at run %d\n", i);
      while(1);
    }

    // now verify and time it
    uint32_t v0 = millis();
    if (mbedtls_rsa_pkcs1_verify(
          &rsa,
          MBEDTLS_MD_SHA256,
          sizeof(hash), hash,
          sig
        ) != 0) {
      Serial.printf("Verify failed at run %d\n", i);
      while(1);
    }
    uint32_t dt = millis() - v0;
    total_verify_ms += dt;

    Serial.printf("Run %3d: verify = %3u ms\n", i, dt);
  }

  float avg = float(total_verify_ms) / NUM_RUNS;
  Serial.printf("\nAverage verify time over %d sigs: %.2f ms\n",
                NUM_RUNS, avg);

  // cleanup
  mbedtls_rsa_free(&rsa);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
}

void loop() {
  // nothing
}
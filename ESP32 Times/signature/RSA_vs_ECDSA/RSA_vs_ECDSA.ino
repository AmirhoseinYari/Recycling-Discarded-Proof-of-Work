#include <Arduino.h>
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/sha256.h"
#include "mbedtls/rsa.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"

const char*  RNG_PERS = "bench";
const int    NUM_IT   = 100;
#define LED_BUILTIN 2

// Common RNG contexts
mbedtls_entropy_context  entropy;
mbedtls_ctr_drbg_context ctr_drbg;

// Buffers for RSA
mbedtls_rsa_context rsa;
uint8_t            rsa_hash[32];
uint8_t            rsa_sig[256];

// Buffers for ECDSA
mbedtls_ecdsa_context ecdsa;
uint8_t               ecdsa_hash[32];
uint8_t               ecdsa_sig[100];
size_t                ecdsa_sig_len;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== HW vs SW Crypto Benchmark ===");

  // 1) seed the DRBG
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  if (mbedtls_ctr_drbg_seed(&ctr_drbg,
        mbedtls_entropy_func, &entropy,
        (const unsigned char*)RNG_PERS, strlen(RNG_PERS)) != 0) {
    Serial.println("ERROR: DRBG seed");
    while (1);
  }

  //
  // 2) PREPARE RSA-2048
  //
  mbedtls_rsa_init(&rsa);                  // <-- single‐arg init
  Serial.print("Gen RSA-2048 keypair… ");
  uint32_t t0 = millis();
  if (mbedtls_rsa_gen_key(
        &rsa,
        mbedtls_ctr_drbg_random, &ctr_drbg,
        2048, 65537) != 0) {
    Serial.println("FAIL");
    while (1);
  }
  Serial.printf("done in %u ms\n", millis() - t0);

  // hash a test message
  const char* msg1 = "Hello RSA";
  mbedtls_sha256((const uint8_t*)msg1, strlen(msg1), rsa_hash, 0);

  // sign once
  Serial.print("RSA sign… ");
  t0 = millis();
  if (mbedtls_rsa_pkcs1_sign(
        &rsa,
        mbedtls_ctr_drbg_random, &ctr_drbg,
        MBEDTLS_MD_SHA256,
        sizeof(rsa_hash), rsa_hash,
        rsa_sig) != 0) {
    Serial.println("SIGN FAIL");
    while (1);
  }
  Serial.printf("done in %u ms\n", millis() - t0);

  // verify once
  Serial.print("RSA verify… ");
  t0 = millis();
  if (mbedtls_rsa_pkcs1_verify(
        &rsa,
        MBEDTLS_MD_SHA256,
        sizeof(rsa_hash), rsa_hash,
        rsa_sig) != 0) {
    Serial.println("VERIFY FAIL");
    while (1);
  }
  Serial.printf("done in %u ms\n", millis() - t0);

  // benchmark NUM_IT verifies
  Serial.printf("RSA %d verifies…\n", NUM_IT*100);
  uint32_t vstart = millis();
  for (int i = 0; i < NUM_IT*100; i++) {
    mbedtls_rsa_pkcs1_verify(
      &rsa,
      MBEDTLS_MD_SHA256,
      sizeof(rsa_hash), rsa_hash,
      rsa_sig);
  }
  uint32_t vend = millis();
  Serial.printf("Total: %u ms, Avg: %.2f ms\n\n",
                vend - vstart,
                float(vend - vstart)/NUM_IT/100);

  mbedtls_rsa_free(&rsa);

  //
  // 3) PREPARE ECDSA-P256
  //
  mbedtls_ecdsa_init(&ecdsa);
  Serial.print("Gen ECDSA-P256 keypair… ");
  t0 = millis();
  if (mbedtls_ecdsa_genkey(
        &ecdsa,
        MBEDTLS_ECP_DP_SECP256R1,
        mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
    Serial.println("FAIL");
    while (1);
  }
  Serial.printf("done in %u ms\n", millis() - t0);

  // hash another message
  const char* msg2 = "Hello ECDSA";
  mbedtls_sha256((const uint8_t*)msg2, strlen(msg2), ecdsa_hash, 0);

  // sign once
  Serial.print("ECDSA sign… ");
  t0 = millis();
  if (mbedtls_ecdsa_write_signature(
        &ecdsa,
        MBEDTLS_MD_SHA256,
        ecdsa_hash, sizeof(ecdsa_hash),
        ecdsa_sig, sizeof(ecdsa_sig), &ecdsa_sig_len,
        mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
    Serial.println("SIGN FAIL");
    while (1);
  }
  Serial.printf("done in %u ms\n", millis() - t0);

  // verify once
  Serial.print("ECDSA verify… ");
  t0 = millis();
  if (mbedtls_ecdsa_read_signature(
        &ecdsa,
        ecdsa_hash, sizeof(ecdsa_hash),
        ecdsa_sig, ecdsa_sig_len) != 0) {
    Serial.println("VERIFY FAIL");
    while (1);
  }
  Serial.printf("done in %u ms\n", millis() - t0);

  // benchmark NUM_IT verifies
  Serial.printf("ECDSA %d verifies…\n", NUM_IT);
  vstart = millis();
  for (int i = 0; i < NUM_IT; i++) {
    mbedtls_ecdsa_read_signature(
      &ecdsa,
      ecdsa_hash, sizeof(ecdsa_hash),
      ecdsa_sig, ecdsa_sig_len);
  }
  vend = millis();
  Serial.printf("Total: %u ms, Avg: %.2f ms\n\n",
                vend - vstart,
                float(vend - vstart)/NUM_IT);

  mbedtls_ecdsa_free(&ecdsa);

  // cleanup RNG
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);

  Serial.println("=== DONE ===");
}

void loop() {
  // nothing
  //digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  //delay(1000);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);
  //Serial.println("xoxo");
}
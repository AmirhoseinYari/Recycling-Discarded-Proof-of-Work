#include <Arduino.h>

// tell wolfSSL where to find its settings & code
#define WOLFSSL_USER_SETTINGS
#include <src/wolfssl/wolfcrypt/settings.h>
#include <src/wolfssl/wolfcrypt/sha256.h>
#include <src/wolfssl/wolfcrypt/rsa.h>
#include <src/wolfssl/wolfcrypt/random.h>


#define NUM_RUNS 100
#define RSA_BITS 2048
#define RSA_EXP  65537

WC_RNG    rng;
RsaKey    rsaKey;

void setup() {
  Serial.begin(115200);
  while(!Serial);

  Serial.println("\n=== wolfCrypt RSA-2048 Verify Benchmark ===");

  // 1) Init RNG
  if (wc_InitRng(&rng) != 0) {
    Serial.println("ERROR: wc_InitRng()");
    while(1);
  }

  // 2) Generate RSA key
  Serial.print("Generating RSA-2048 key… ");
  uint32_t t0 = millis();
  wc_InitRsaKey(&rsaKey, NULL);
  if (wc_MakeRsaKey(&rsaKey, RSA_BITS, RSA_EXP, &rng) != 0) {
    Serial.println("ERROR: wc_MakeRsaKey()");
    while(1);
  }
  Serial.printf("done in %u ms\n\n", millis() - t0);

  // Buffers
  uint8_t  hashOut[SHA256_DIGEST_SIZE];
  uint8_t  sig[256];
  word32   sigLen;
  uint8_t  verifyBuf[256];
  word32   verifyLen;

  uint32_t total_verify_ms = 0;

  // 3) Repeat: hash, sign, then time verify for 100 distinct messages
  for (int i = 0; i < NUM_RUNS; i++) {
    // a) Unique message
    char msg[32];
    int  mlen = snprintf(msg, sizeof(msg), "Message #%d", i);

    // b) Hash it
    wc_Sha256Hash((const byte*)msg, mlen, hashOut);

    // c) Sign it (we don’t time this)
    sigLen = sizeof(sig);
    if (wc_RsaSSL_Sign(hashOut, sizeof(hashOut),
                       sig, &sigLen,
                       &rsaKey, &rng) != 0) {
      Serial.printf("ERROR: Sign #%d\n", i);
      while(1);
    }

    // d) Verify & time it
    uint32_t v0 = millis();
    verifyLen = sizeof(verifyBuf);
    if (wc_RsaSSL_Verify(sig, sigLen,
                         verifyBuf, &verifyLen,
                         &rsaKey) != 0) {
      Serial.printf("ERROR: Verify #%d\n", i);
      while(1);
    }
    uint32_t dt = millis() - v0;
    total_verify_ms += dt;

    Serial.printf("Run %3d: verify = %3u ms\n", i, dt);
  }

  float avg = float(total_verify_ms) / NUM_RUNS;
  Serial.printf("\nAverage verify time over %d runs: %.2f ms\n",
                NUM_RUNS, avg);

  // Cleanup
  wc_FreeRsaKey(&rsaKey);
  wc_FreeRng(&rng);
}

void loop() {
  // nothing
}
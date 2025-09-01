#include <Arduino.h>
#include "mbedtls/sha256.h"
#include "esp32/sha.h"
#include "esp_system.h"

static const size_t DATA_SIZE = 32 * 1024;  // 32 KB
static uint8_t data[DATA_SIZE];
uint8_t hash_mbedtls[32];
uint8_t hash_hw[32];

const int NUM_RUNS = 100;

void calc_sha256_mbedtls(const uint8_t *input, size_t ilen, uint8_t output[32]) {
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, input, ilen);
  mbedtls_sha256_finish(&ctx, output);
  mbedtls_sha256_free(&ctx);
}

void calc_sha256_hw(const uint8_t *input, size_t ilen, uint8_t output[32]) {
  esp_sha(SHA2_256, input, ilen, output);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.printf("SHA-256 Benchmark: %d×(%u KB)\n\n", NUM_RUNS, DATA_SIZE / 1024);

  // Fill the buffer with dummy data
  for (size_t i = 0; i < DATA_SIZE; i++) {
    data[i] = i & 0xFF;
  }

  // --- mbedTLS SHA-256 ---
  uint32_t t0 = millis();
  for (int i = 0; i < NUM_RUNS; i++) {
    calc_sha256_mbedtls(data, DATA_SIZE, hash_mbedtls);
  }
  uint32_t t1 = millis();
  uint32_t dt_mbed = t1 - t0;
  float avg_mbed = dt_mbed / float(NUM_RUNS);

  Serial.printf("mbedTLS: total %u ms for %d runs, avg %.2f ms/run\n",
                dt_mbed, NUM_RUNS, avg_mbed);

  // --- Hardware SHA-256 ---
  t0 = millis();
  for (int i = 0; i < NUM_RUNS; i++) {
    calc_sha256_hw(data, DATA_SIZE, hash_hw);
  }
  t1 = millis();
  uint32_t dt_hw = t1 - t0;
  float avg_hw = dt_hw / float(NUM_RUNS);

  Serial.printf("HW     : total %u ms for %d runs, avg %.2f ms/run\n\n",
                dt_hw, NUM_RUNS, avg_hw);

  // Verify last hash matches
  bool same = true;
  for (int i = 0; i < 32; i++) {
    if (hash_mbedtls[i] != hash_hw[i]) {
      same = false;
      break;
    }
  }
  Serial.println(same ? "Outputs MATCH" : "Outputs DO NOT match");
}

void loop() {
  // nothing
}
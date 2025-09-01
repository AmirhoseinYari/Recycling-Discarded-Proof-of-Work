#include <Arduino.h>
#include "esp32/sha.h"
#include "esp_system.h"

static const int   BATCHES     = 1000;
static const int   PER_BATCH   = 26;             // 26 small hashes per batch
static const size_t SIZE_SMALL = 32;             // 32 bytes per small hash
static const size_t SIZE_LARGE = PER_BATCH * SIZE_SMALL;  // 832 bytes per large hash

uint8_t data_small[SIZE_SMALL];
uint8_t data_large[SIZE_LARGE];
uint8_t hash_out[32];

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== SHA-256: 1000×(26×32 B) vs 1000×832 B ===");

  // initialize data patterns
  for (size_t i = 0; i < SIZE_SMALL; i++) {
    data_small[i] = uint8_t(i);
  }
  for (size_t i = 0; i < SIZE_LARGE; i++) {
    data_large[i] = uint8_t(i);
  }

  // --- Benchmark 1000 batches of 26×32 B hashes ---
  uint32_t t0 = millis();
  for (int b = 0; b < BATCHES; b++) {
    for (int i = 0; i < PER_BATCH; i++) {
      esp_sha(SHA2_256, data_small, SIZE_SMALL, hash_out);
    }
  }
  uint32_t dt_small_batches = millis() - t0;
  float    avg_batch_small  = float(dt_small_batches) / BATCHES;
  float    avg_hash_small   = float(dt_small_batches) / (BATCHES * PER_BATCH);
  Serial.printf("1000×(26×%2u B) : total %4u ms, avg %6.3f ms/batch, %6.3f ms/hash\n",
                unsigned(SIZE_SMALL), dt_small_batches, avg_batch_small, avg_hash_small);

  // --- Benchmark 1000× single 832 B hashes ---
  t0 = millis();
  for (int b = 0; b < BATCHES; b++) {
    esp_sha(SHA2_256, data_large, SIZE_LARGE, hash_out);
  }
  uint32_t dt_large = millis() - t0;
  float    avg_hash_large = float(dt_large) / BATCHES;
  Serial.printf("1000×%3u B   : total %4u ms, avg %6.3f ms/hash\n\n",
                unsigned(SIZE_LARGE), dt_large, avg_hash_large);

  // sanity check: totals should be nearly equal
  if (abs(int(dt_small_batches) - int(dt_large)) <= 1) {
    Serial.println("Totals match within 1 ms");
  } else {
    Serial.println("Totals differ >1 ms");
  }
}

void loop() {
  // nothing
}
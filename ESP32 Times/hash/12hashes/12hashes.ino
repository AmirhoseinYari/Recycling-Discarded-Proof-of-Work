#include "esp32/sha.h"
#include "esp_system.h"

// 13 inputs of 32 bytes (256 bits) each
static uint8_t inputs[12][32];
uint8_t hash_out[32];
uint8_t tmp_hash[32];

// Perform SHA256d (double-SHA256) in hardware
void sha256d_hw(const uint8_t *in, size_t len, uint8_t out[32]) {
  esp_sha(SHA2_256, in, len, tmp_hash);   // first SHA-256
  esp_sha(SHA2_256, tmp_hash, 32, out);   // second SHA-256
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize the 13 inputs (simple structure of input)
  for (int j = 0; j < 13; j++) {
    for (int i = 0; i < 32; i++) {
      inputs[j][i] = (uint8_t)((j * 32 + i) & 0xFF);
    }
  }

  Serial.println("SHA256d Hardware Benchmark: 100×(13×32 B)");

  // Benchmark 1000 batches of 13 sha256d calls
  const int batches = 1000;
  uint32_t start = millis();
  for (int b = 0; b < batches; b++) {
    for (int j = 0; j < 13; j++) {
      sha256d_hw(inputs[j], 32, hash_out);
    }
  }
  uint32_t end = millis();

  uint32_t totalMs = end - start;
  float avgPerBatch = totalMs / float(batches);

  Serial.printf("Total: %u ms for %u batches (%u hashes)\n",
                totalMs, batches, batches * 13);
  Serial.printf("Avg per batch (13 hashes): %.2f ms\n", avgPerBatch);

}

void loop() {
  // nothing     
}
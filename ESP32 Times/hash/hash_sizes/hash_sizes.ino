#include <Arduino.h>
#include "esp32/sha.h"

#define LED_BUILTIN 2

// How many times to hash each size to get an average
static const int RUNS_PER_SIZE = 1000;

// List of buffer sizes to test (in bytes)
static const size_t sizes[] = {
  32, 64, 128, 256, 512, 1024,
  2048, 4096, 8192, 16384, 32768
};
static const int NUM_SIZES = sizeof(sizes)/sizeof(sizes[0]);

// Max buffer needed
static const size_t MAX_SIZE = 32768;

// Pre-allocate one big buffer
static uint8_t buffer[MAX_SIZE];
static uint8_t hash_out[32];

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while(!Serial);

  // Fill the buffer once with a pattern
  for (size_t i = 0; i < MAX_SIZE; i++) {
    buffer[i] = uint8_t(i);
  }

  Serial.println("size_bytes,avg_time_ms");
  for (int idx = 0; idx < NUM_SIZES; idx++) {
    size_t N = sizes[idx];

    // Warm up
    esp_sha(SHA2_256, buffer, N, hash_out);

    // Time RUNS_PER_SIZE runs
    uint32_t t0 = millis();
    for (int r = 0; r < RUNS_PER_SIZE; r++) {
      esp_sha(SHA2_256, buffer, N, hash_out);
    }
    uint32_t dt = millis() - t0;
    float avg = float(dt) / RUNS_PER_SIZE;

    // Print CSV line
    Serial.printf("%u,%.4f\n", unsigned(N), avg);
    delay(50);  // give serial a moment
  }

  Serial.println("Done.");
}

void loop() {
  // nothing
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);
  Serial.println("xoxo");
}
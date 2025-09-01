#include <WiFi.h>
#include "mbedtls/sha256.h"   // For the hashing task

// --- IMPORTANT: Replace with your home Wi-Fi credentials ---
const char* ssid = "SPSETUP-D2F6";
const char* password = "count9525candid";
// ---

// Hashing function to make the CPU busy
void performHashing() {
  byte shaResult[32];
  char* test_data = "This is some data to hash for the power test.";

  // Run the hash many times to keep the CPU busy for a few seconds
  for (int i = 0; i < 200000; i++) {
    mbedtls_sha256((const unsigned char*)test_data, strlen(test_data), shaResult, 0);
  }
}

// Function to connect to a standard home Wi-Fi
void connectToWifi() {
  Serial.print("Connecting to network: ");
  Serial.println(ssid);

  // For home Wi-Fi, you only need WiFi.begin(ssid, password)
  WiFi.begin(ssid, password);

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    if (counter >= 60) { // Timeout after 30 seconds
      Serial.println("\nFailed to connect. Restarting...");
      delay(1000);
      ESP.restart();
    }
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n--- ESP32 Power Test ---");
  connectToWifi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Connection lost! Reconnecting...");
      connectToWifi();
  }

  // --- STATE 1: Normal Mode (Wi-Fi Idle) ---
  Serial.println("\n----------------------------------------");
  Serial.println("STATE 1: Normal Mode (Wi-Fi Idle). Measuring for 10 seconds...");
  delay(10000); // Wait 10 seconds

  // --- STATE 2: Hashing (CPU Intensive, Wi-Fi modem sleep) ---
  Serial.println("\n----------------------------------------");
  Serial.println("STATE 2: Hashing Task (Wi-Fi ON)...");
  
  unsigned long startTime2 = millis();
  performHashing();
  unsigned long duration2 = millis() - startTime2;
  Serial.print("Hashing finished in ");
  Serial.print(duration2);
  Serial.println(" ms.");
  delay(10000); // Wait 10 seconds

  // --- STATE 3: Wi-Fi Radio OFF ---
  Serial.println("\n----------------------------------------");
  Serial.println("STATE 3: Turning OFF Wi-Fi Radio. Measuring for 10 seconds...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(10000); // Wait 10 seconds

  // --- STATE 4: Hashing (CPU Intensive, Wi-Fi OFF) ---
  Serial.println("\n----------------------------------------");
  Serial.println("STATE 4: Hashing Task (Wi-Fi OFF)...");
  
  unsigned long startTime4 = millis();
  performHashing();
  unsigned long duration4 = millis() - startTime4;
  Serial.print("Hashing finished in ");
  Serial.print(duration4);
  Serial.println(" ms.");
  delay(10000); // Wait 10 seconds

  // --- Reconnecting to start the cycle over ---
  Serial.println("\n----------------------------------------");
  Serial.println("Re-enabling Wi-Fi and reconnecting...");
  connectToWifi(); // Use our simplified connection function
}
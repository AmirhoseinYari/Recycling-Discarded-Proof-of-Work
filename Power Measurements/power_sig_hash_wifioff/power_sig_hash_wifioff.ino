// Include libraries for Hashing, ECDSA, and RSA from mbedTLS
#include <WiFi.h>
#include "mbedtls/sha256.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#define LED_BUILTIN 2

// --- Arithmetic Function ---
void performArithmetic() {
  Serial.println("--- Starting Arithmetic Operations ---");
  volatile long result = 0;
  long a = 123456789;
  int b = 987654321;
  const int numOperations = 100000000; // Adjust this number for your needs

  unsigned long startTime = millis();
  
  // Perform a simple arithmetic loop many times
  for (int i = 0; i < numOperations; i++) {
    result += (a * b) / (i + 1); // A simple set of operations
  }
  
  unsigned long duration = millis() - startTime;
  Serial.print("Arithmetic finished in: ");
  Serial.print(duration);
  Serial.println(" ms");

  // To prevent the compiler from optimizing the loop away,
  // we print the final result.
  Serial.print("Final result (volatile): ");
  Serial.println(result);
}

// --- Hashing Function ---
void performHashing() {
  Serial.println("--- Starting Hashing (SHA-256) ---");
  byte shaResult[32];
  char* test_data = "This is some data to hash for the power test.";
  
  unsigned long startTime = millis();
  // Run the hash many times to get a measurable workload
  for (int i = 0; i < 200000; i++) {
    mbedtls_sha256((const unsigned char*)test_data, strlen(test_data), shaResult, 0);
  }
  unsigned long duration = millis() - startTime;
  Serial.print("Hashing finished in: ");
  Serial.print(duration);
  Serial.println(" ms");
}

// --- ECDSA Function ---
void performEcdsa() {
  Serial.println("\n--- Starting ECDSA (SECP256R1) ---");
  unsigned long startTime = millis();

  // mbedTLS requires a random number generator (RNG) for crypto operations
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);

  // ECDSA variables
  mbedtls_ecdsa_context ecdsa;
  byte hash[32];
  byte sig[MBEDTLS_MPI_MAX_SIZE];
  size_t sig_len;
  int success_count = 0;
  const int NUM_ECDSA_VERIFICATIONS = 100; // New: Number of ECDSA verifications

  // 1. Generate a new key pair (do this once outside the loop for efficiency)
  mbedtls_ecdsa_init(&ecdsa);
  mbedtls_ecdsa_genkey(&ecdsa, MBEDTLS_ECP_DP_SECP256R1, mbedtls_ctr_drbg_random, &ctr_drbg);

  // 2. Sign a hash (do this once outside the loop)
  mbedtls_sha256((const unsigned char*)"message to sign", 15, hash, 0);
  mbedtls_ecdsa_write_signature(&ecdsa, MBEDTLS_MD_SHA256, hash, sizeof(hash), sig, sizeof(sig), &sig_len, mbedtls_ctr_drbg_random, &ctr_drbg);

  // 3. Verify the signature multiple times
  for (int i = 0; i < NUM_ECDSA_VERIFICATIONS; i++) {
    int result = mbedtls_ecdsa_read_signature(&ecdsa, hash, sizeof(hash), sig, sig_len);
    if (result == 0) {
      success_count++;
    }
  }
  
  // Clean up
  mbedtls_ecdsa_free(&ecdsa);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
  
  unsigned long duration = millis() - startTime;
  Serial.print("ECDSA (Keygen, Sign once, ");
  Serial.print(NUM_ECDSA_VERIFICATIONS);
  Serial.print(" Verifications) finished in: ");
  Serial.print(duration);
  Serial.println(" ms");
  Serial.print("ECDSA Verification: ");
  Serial.print(success_count);
  Serial.print("/");
  Serial.print(NUM_ECDSA_VERIFICATIONS);
  Serial.println(" SUCCESS");
}

// --- RSA Function ---
void performRsa() {
  Serial.println("\n--- Starting RSA (2048-bit) ---");
  unsigned long startTime = millis();
  
  // Set up RNG
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);

  // RSA variables
  mbedtls_rsa_context rsa;
  byte hash[32];
  byte sig[256]; // 2048 bits = 256 bytes
  int success_count = 0;
  const int NUM_RSA_VERIFICATIONS = 10000; // New: Number of RSA verifications

  // 1. Generate a new key pair (do this once outside the loop for efficiency)
  mbedtls_rsa_init(&rsa);
  mbedtls_rsa_gen_key(&rsa, mbedtls_ctr_drbg_random, &ctr_drbg, 2048, 65537);

  // 2. Sign a hash (do this once outside the loop)
  mbedtls_sha256((const unsigned char*)"message to sign", 15, hash, 0);
  mbedtls_rsa_pkcs1_sign(&rsa, mbedtls_ctr_drbg_random, &ctr_drbg, MBEDTLS_MD_SHA256, sizeof(hash), hash, sig);

  // 3. Verify the signature multiple times
  for (int i = 0; i < NUM_RSA_VERIFICATIONS; i++) {
    int result = mbedtls_rsa_pkcs1_verify(&rsa, MBEDTLS_MD_SHA256, sizeof(hash), hash, sig);
    if (result == 0) {
      success_count++;
    }
  }
  
  // Clean up
  mbedtls_rsa_free(&rsa);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);

  unsigned long duration = millis() - startTime;
  Serial.print("RSA (Keygen, Sign once, ");
  Serial.print(NUM_RSA_VERIFICATIONS);
  Serial.print(" Verifications) finished in: ");
  Serial.print(duration);
  Serial.println(" ms");
  Serial.print("RSA Verification: ");
  Serial.print(success_count);
  Serial.print("/");
  Serial.print(NUM_RSA_VERIFICATIONS);
  Serial.println(" SUCCESS");
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay(100);
  Serial.println("\n--- Crypto Power Comparison ---");
  
  // Turn Wi-Fi OFF immediately for all tests
  WiFi.mode(WIFI_OFF);

  Serial.println("Wi-Fi is OFF.");
  for(int i=0; i<10; i++){
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(1000);                      // wait for a second
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    delay(1000);
    Serial.println("xoxo");
  }
  
}

void loop() {
  // Perform each cryptographic task and wait, allowing you to measure power
  performArithmetic();
  delay(5000);

  performHashing();
  delay(5000);

  performEcdsa();
  delay(5000);
  
  performRsa();
  delay(15000); // Longer delay after the slow RSA operation
}
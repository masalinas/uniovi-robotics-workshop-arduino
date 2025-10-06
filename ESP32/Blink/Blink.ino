// Simple Blink example for ESP32-WROOM-32D
// Works with the "esp32 by Espressif Systems" Arduino core

#define LED_PIN 2  // Built-in LED is usually on GPIO 2

void setup() {
  pinMode(LED_PIN, OUTPUT);  // Set pin as output
}

void loop() {
  digitalWrite(LED_PIN, HIGH);  // Turn LED on
  delay(1000);                  // Wait 1 second
  digitalWrite(LED_PIN, LOW);   // Turn LED off
  delay(1000);                  // Wait 1 second
}

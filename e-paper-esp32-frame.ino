#include "esp_sleep.h"
#define LED_PIN 2 // Change this to the GPIO pin connected to your LED

RTC_DATA_ATTR int bootCount = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT); // Set the LED pin as output

  Serial.begin(115200);
  delay(1000); // Allow time for serial communication to initialize

  // Increase boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  if (bootCount == 1) {
    Serial.println("Going into deep sleep now");
    esp_deep_sleep(1000000LL * 20); // Sleep for 20 seconds
  } else {
    digitalWrite(LED_PIN, HIGH); // Turn on the LED
  }
}

void loop() {
  // This will never be reached, as the ESP32 will go into deep sleep in the setup() function
}
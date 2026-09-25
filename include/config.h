#pragma once
#include <Arduino.h>

// Analog input: GPIO34 is ADC1_CH6 (input only, usable while Wi-Fi is active).
constexpr uint8_t PROBE_PIN = 34;

// Built-in 1 kHz square wave on GPIO25 for probe calibration.
constexpr uint8_t CAL_PIN = 25;
constexpr uint32_t CAL_FREQ_HZ = 1000;

constexpr size_t FRAME_SAMPLES = 512;            // samples returned per capture
constexpr size_t PRETRIGGER = FRAME_SAMPLES / 4;  // samples kept before the trigger point
constexpr uint32_t MAX_SAMPLE_RATE = 50000;      // Hz, limited by analogRead() on the ESP32
constexpr uint32_t MIN_SAMPLE_RATE = 100;
constexpr uint32_t TRIGGER_TIMEOUT_MS = 250;     // auto mode: free-run if no trigger

constexpr const char* AP_SSID = "ESP32-Scope";
constexpr const char* AP_PASSWORD = "scope1234";  // change before deployment

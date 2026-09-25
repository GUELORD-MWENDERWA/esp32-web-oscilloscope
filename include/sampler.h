#pragma once
#include <Arduino.h>

#include "config.h"

enum class TriggerEdge : uint8_t { Rising, Falling, None };

struct CaptureSettings {
  uint32_t sampleRate = 20000;
  uint16_t triggerLevel = 2048;  // raw 12-bit ADC value
  TriggerEdge edge = TriggerEdge::Rising;
};

struct Frame {
  uint16_t samples[FRAME_SAMPLES];
  uint32_t sampleRate;
  bool triggered;
};

// Paced ADC sampler with a ring buffer, edge trigger with hysteresis,
// pre-trigger history and an auto (free-run) timeout.
class Sampler {
 public:
  void begin();
  // Blocks until a frame is complete. Returns true when a real trigger was
  // found, false when the frame is free-running (no trigger before timeout).
  bool capture(const CaptureSettings& settings, Frame& out);
};

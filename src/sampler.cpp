#include "sampler.h"

namespace {

constexpr size_t RING = 1024;  // power of two, must hold one full frame
static_assert((RING & (RING - 1)) == 0, "ring size must be a power of two");
static_assert(RING >= FRAME_SAMPLES, "ring must hold a frame");

uint16_t ring[RING];

}  // namespace

void Sampler::begin() {
  analogReadResolution(12);
  analogSetPinAttenuation(PROBE_PIN, ADC_11db);  // about 0 to 3.1 V full scale
}

// Sampling is paced with micros() in a tight loop rather than from a timer
// interrupt: analogRead() is not safe to call from an ISR on the Arduino-ESP32
// core. The HTTP request that asked for the capture is waiting anyway, so
// blocking here costs nothing. Wi-Fi interrupts add a few microseconds of jitter.
bool Sampler::capture(const CaptureSettings& s, Frame& out) {
  const uint32_t rate = constrain(s.sampleRate, MIN_SAMPLE_RATE, MAX_SAMPLE_RATE);
  const uint32_t periodUs = 1000000UL / rate;
  out.sampleRate = 1000000UL / periodUs;  // actual rate after integer rounding
  out.triggered = false;

  // Hysteresis: the signal must first move clearly away from the level before
  // a crossing is accepted, which rejects noise around the trigger level.
  const int hyst = 40;
  const int level = s.triggerLevel;
  const uint32_t timeoutUs = TRIGGER_TIMEOUT_MS * 1000UL;

  uint32_t n = 0;              // samples taken
  int64_t triggerAt = -1;      // sample index of the trigger point
  bool armed = false;
  const uint32_t t0 = micros();
  uint32_t next = t0;

  while (true) {
    while (static_cast<int32_t>(micros() - next) < 0) {
    }
    next += periodUs;
    const int v = analogRead(PROBE_PIN);
    ring[n & (RING - 1)] = v;
    n++;

    if (triggerAt < 0 && n > PRETRIGGER) {
      bool hit = false;
      switch (s.edge) {
        case TriggerEdge::None:
          hit = true;
          break;
        case TriggerEdge::Rising:
          if (v < level - hyst) armed = true;
          else if (armed && v >= level) hit = true;
          break;
        case TriggerEdge::Falling:
          if (v > level + hyst) armed = true;
          else if (armed && v <= level) hit = true;
          break;
      }
      if (hit) triggerAt = n - 1;
      else if (micros() - t0 > timeoutUs) triggerAt = n - 1;  // auto mode: free-run frame
      out.triggered = hit && s.edge != TriggerEdge::None;
    }

    if (triggerAt >= 0 && n >= static_cast<uint32_t>(triggerAt) - PRETRIGGER + FRAME_SAMPLES) break;
  }

  const uint32_t first = static_cast<uint32_t>(triggerAt) - PRETRIGGER;
  for (size_t i = 0; i < FRAME_SAMPLES; i++) out.samples[i] = ring[(first + i) & (RING - 1)];
  return out.triggered;
}

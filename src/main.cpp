// ESP32 Web Oscilloscope
// Samples an analog input with a hardware timer, finds a trigger point in a ring
// buffer, and serves captured frames as JSON to a browser-based scope display.

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "sampler.h"
#include "web_ui.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

WebServer server(80);
Sampler sampler;
Frame frame;

void handleCapture() {
  CaptureSettings s;
  if (server.hasArg("rate")) s.sampleRate = server.arg("rate").toInt();
  if (server.hasArg("level")) s.triggerLevel = constrain(server.arg("level").toInt(), 0, 4095);
  const String edge = server.arg("edge");
  s.edge = edge == "falling" ? TriggerEdge::Falling : edge == "none" ? TriggerEdge::None : TriggerEdge::Rising;

  const bool triggered = sampler.capture(s, frame);

  // Build the JSON by hand: about 2.6 kB, no JSON library needed.
  String json;
  json.reserve(FRAME_SAMPLES * 5 + 64);
  json += "{\"rate\":";
  json += frame.sampleRate;
  json += ",\"triggered\":";
  json += triggered ? "true" : "false";
  json += ",\"samples\":[";
  for (size_t i = 0; i < FRAME_SAMPLES; i++) {
    if (i) json += ',';
    json += frame.samples[i];
  }
  json += "]}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void startNetwork() {
  if (strlen(WIFI_SSID) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("Connecting to %s", WIFI_SSID);
    for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("Open http://%s/\n", WiFi.localIP().toString().c_str());
      return;
    }
    Serial.println("Station mode failed, falling back to access point");
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("Access point %s, open http://%s/\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Calibration signal: 1 kHz, 50 % duty square wave from the LEDC peripheral.
  ledcSetup(0, CAL_FREQ_HZ, 8);
  ledcAttachPin(CAL_PIN, 0);
  ledcWrite(0, 128);

  sampler.begin();
  startNetwork();

  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/capture", HTTP_GET, handleCapture);
  server.onNotFound([] { server.send(404, "text/plain", "not found"); });
  server.begin();
}

void loop() { server.handleClient(); }

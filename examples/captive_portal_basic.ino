#include "ESPPortal.h"
#include <iostream>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
// #include "FS.h"
#include <string>
#include <Arduino.h>

ESPPortal p;

void setup() {
  Serial.begin(115200);

  Serial.println("");
  Serial.println("Setup, begin");
  
  p.begin();
}

void loop() {
  
}

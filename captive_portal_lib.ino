/**
 * captive_portal_lib.ino
 * 
 * ESPPortal, a library for the ESP8266/Arduino platform
 * for configuration of WiFi credentials 
 * using wildcard dns ap to serve a basic config page.
 * 
 * @author Kurt Johnson
 * @author kj32
 * @version 0.0.0
 * @license MIT
 */

#include <Arduino.h>
#include "src/ESPPortal.hpp"
#include <iostream>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <string>

ESPPortal p;

void setup() {
  Serial.begin(115200);

  Serial.println("");
  Serial.println("Setup, begin");

  p.begin();
  
}

void loop() {
  p.listen();
}

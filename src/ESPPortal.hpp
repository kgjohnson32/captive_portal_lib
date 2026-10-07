/**
 * ESPPortal.hpp
 * 
 * ESPPortal, a library for the ESP8266/Arduino platform
 * for configuration of WiFi credentials using a Captive Portal
 * 
 * @author Kurt Johnson
 * @author kj32
 * @version 0.0.0
 * @license MIT
 */

#ifndef ESP_PORTAL_hpp
#define ESP_PORTAL_hpp

#include <iostream>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <string>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

extern const char LOGIN_FORM[] PROGMEM;

class ESPPortal
{
	private:
		const int DNS_PORT 			= 53; 
		const char* AP_SSID 		= "Board-setup";
		const char* CONFIG_FILE = "wifi_cred.json";
		int isSetup							= 0;
		IPAddress apID;
		DNSServer dnsServer;
		ESP8266WebServer server;
		JsonDocument doc;
		WiFiMode_t _mode;

	public:
		ESPPortal();
		void begin();
		void handleRoot();
		void handleNotFound();
		bool hasCredentials();
		void handleHttpReset();
		void handleFileList();
		bool loadCredentials();
		void clearCredentialsAndReset();
		void saveCredentials(const char* ssid, const char* pass);
		void clearCredsReset();
		void handleSave();
		void readTextFile();
		void WIFIconnect(WiFiMode_t m, bool mapRoutes);
		void listen();
		wl_status_t getWiFiStatus();
};

#endif
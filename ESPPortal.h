#ifndef ESP_PORTAL_h
#define ESP_PORTAL_h

#include <string>
#include <Arduino.h>
#include <iostream>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include "FS.h"
#include <ArduinoJson.h>

extern const char LOGIN_FORM[] PROGMEM;

class ESPPortal
{
	private:
		const int DNS_PORT 			= 53; 
		const char* AP_SSID 		= "BOARD-SETUP";
		const char* CONFIG_FILE = "wifi_cred.txt";
		DNSServer dnsServer;
		ESP8266WebServer server;
		JsonDocument doc;
		int isSetup							= 0;

	protected:
		IPAddress apID;

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
};

#endif
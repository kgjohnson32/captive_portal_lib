/**
 * ESPPortal.cpp
 * 
 * ESPPortal, a library for the ESP8266/Arduino platform
 * for configuration of WiFi credentials using a Captive Portal
 * 
 * @author Kurt Johnson
 * @author kj32
 * @version 0.0.0
 * @license MIT
 */

#include <Arduino.h>
#include "ESPPortal.hpp"
#include <iostream>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <string>
#include "ArduinoJson/ArduinoJson-v7.4.3.h"

const char LOGIN_FORM[] = R"rawhtml(
		<!DOCTYPE html>
		<html>
		<head>
				<meta name="viewport" content="width=device-width, initial-scale=1.0">
				<title>ESP12 Wi-Fi Setup</title>
				<style>
						body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f9; color: #333; }
						.container { max-width: 400px; margin: 0 auto; background: white; padding: 25px; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
						h2 { margin-top: 0; color: #0076ff; text-align: center; }
						label { display: block; margin: 12px 0 6px; font-weight: bold; }
						input[type="text"], input[type="password"] { width: 100%; padding: 10px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; }
						input[type="submit"] { width: 100%; padding: 12px; margin-top: 20px; background-color: #0076ff; border: none; border-radius: 4px; color: white; font-size: 16px; cursor: pointer; }
						input[type="submit"]:hover { background-color: #0056b3; }
				</style>
		</head>
		<body>
				<div class="container">
						<h2>Wi-Fi Configuration</h2>
						<form action="/save" method="POST">
								<label for="ssid">Network Name (SSID):</label>
								<input type="text" id="ssid" name="ssid" placeholder="Enter Wi-Fi Name" required>
								<label for="password">Password:</label>
								<input type="password" id="password" name="password" placeholder="Enter Wi-Fi Password">
								<input type="submit" value="Save & Connect">
						</form>
				</div>
		</body>
		</html>
		)rawhtml";

char* sta_ssid;
char* sta_pass;

int isSetup = 0;

#define DEBUG false

#if DEBUG 
  #define DEBUG_PRINT(a) Serial.print(a)
  #define DEBUG_PRINTLN(a) Serial.println(a)
#else 
  #define DEBUG_PRINT(a)
  #define DEBUG_PRINTLN(a)
#endif

const char* CONFIG_FILE = "wifi_config.json";

// Object Declarations
IPAddress apID(192, 168, 4, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);

ESPPortal::ESPPortal() {
  DEBUG_PRINTLN("Created New Captive Portal\n");
}

void ESPPortal::handleRoot() {
  DEBUG_PRINTLN("Handle root");
  server.send_P(200, "text/html", LOGIN_FORM);
}

bool ESPPortal::hasCredentials() {
  DEBUG_PRINTLN("File check"); // If the file exists, we may have creds.

  File configFile = LittleFS.open(CONFIG_FILE, "r");  
  if (!configFile) {
    DEBUG_PRINTLN(" - >> Does not have credentails .......");
    DEBUG_PRINTLN(configFile ? true : false);
  }

  return configFile ? true : false;
}

void ESPPortal::saveCredentials(const char* ssid, const char* pass) {
  DEBUG_PRINTLN("saveCredentials ");
  DEBUG_PRINT(ssid);
  DEBUG_PRINT(" " );
  DEBUG_PRINT(pass);

  File file = LittleFS.open(CONFIG_FILE, "w");

  if (!file) {
    DEBUG_PRINTLN("No file");
    return; 
  }
  DEBUG_PRINTLN("File exists.");

  doc["sta_ssid"] = ssid;
  doc["sta_pass"] = pass;
  doc["sta_secu"] = "";
  serializeJson(doc, file);
  file.flush();
  file.close(); 

  DEBUG_PRINTLN("[FS] Credentials saved to flash!");
}

void ESPPortal::handleNotFound() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  server.send(302, "text/plain", "Redirecting to captive portal");
}

void ESPPortal::clearCredsReset() {
  DEBUG_PRINTLN("[RESET] Erasing Wi-Fi configurations...");

  if (LittleFS.exists(CONFIG_FILE)) {
    LittleFS.remove(CONFIG_FILE);
  }
}

void ESPPortal::listen() {
    server.handleClient();
    delay(30);
}

void ESPPortal::WIFIconnect(WiFiMode_t mode, bool mapRoutes = true) { 
  _mode = mode;
  WiFi.mode(_mode);
  WiFi.begin(sta_ssid, sta_pass);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    DEBUG_PRINT(".");
    attempts++;
  }

  if (mapRoutes) {
    server.on("/",          HTTP_GET, [this]() { this->handleRoot();        } );
    server.on("/list",      HTTP_GET, [this]() { this->handleFileList();    } );
    server.on("/list/file", HTTP_GET, [this]() { this->readTextFile();      } );
    server.on("/reset",     HTTP_GET, [this]() { this->handleHttpReset();   } );
  }

  server.begin();
  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("Server Started");
   
  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINTLN("IN STA MODE, Connected");
    DEBUG_PRINT("IP Address: ");
    DEBUG_PRINTLN(WiFi.localIP());
  }
}

void ESPPortal::begin() {
    Serial.begin(115200);
    
    LittleFS.begin();
    delay(20);

    DEBUG_PRINTLN("Begin");
      
    if (hasCredentials() == false) {
      DEBUG_PRINTLN("\nInitializing Scoreboard Captive Portal...");

      // Access Point
      WiFi.mode(WIFI_AP);
      WiFi.softAP(AP_SSID);

      delay(10);
      
      IPAddress apIP = WiFi.softAPIP();
      DEBUG_PRINT("Access Point Created. SSID: "); 
      DEBUG_PRINTLN(AP_SSID);

      DEBUG_PRINT("IP Address: "); 
      DEBUG_PRINTLN(apIP);

      DEBUG_PRINT("DNS Port ");
      DEBUG_PRINTLN(DNS_PORT);

      // Override DNS queries (*) to the ESP IP
      dnsServer.start(DNS_PORT, "*", apIP);
      DEBUG_PRINTLN("DNS Sever started"); 

      server.on("/",      HTTP_GET,   [this]() { this->handleRoot(); } );
      DEBUG_PRINTLN("Route: Root registered");
      server.on("/save",  HTTP_POST,  [this]() { this->handleSave(); } );
      DEBUG_PRINTLN("Route: Save registered");

      server.onNotFound([this]() { this->handleNotFound(); } );
      DEBUG_PRINTLN("Route: Not Found registered");

      DEBUG_PRINTLN("Starting server");
      server.begin();
      DEBUG_PRINTLN("Server Started");

      DEBUG_PRINTLN("Preparing AP");

      while(isSetup == 0) {
 
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {

          dnsServer.processNextRequest();
          server.handleClient();

        } else {
          dnsServer.stop();
          isSetup = 1; 
        }
        
        delay(30);
      }

    } else {

      loadCredentials();

      if (sta_ssid == nullptr) {
        DEBUG_PRINTLN("Did not parse file.");
        return;
      }

      DEBUG_PRINT("Connecting using stored creds ");
      DEBUG_PRINTLN(sta_ssid);
      DEBUG_PRINTLN(sta_pass);

      WiFiMode_t currentMode = WIFI_STA;
      WIFIconnect(currentMode, true);
    }

    while(isSetup == 1 && WiFi.status() == WL_CONNECTED) {

      server.handleClient();
      delay(30);
    }

}

void ESPPortal::clearCredentialsAndReset() {
  DEBUG_PRINTLN("[RESET] Erasing Wi-Fi configurations...");

  if (LittleFS.exists(CONFIG_FILE)) {
    DEBUG_PRINTLN("Config file removed.");
    LittleFS.remove(CONFIG_FILE);
  }

  if (LittleFS.exists("wifi_cred.txt")) {
    DEBUG_PRINTLN("Config file removed.");
    LittleFS.remove("wifi_cred.txt");
  }

  WiFi.disconnect(true); // Erase SDK cached credentials as well
  delay(1000);
  
  DEBUG_PRINTLN("[RESET] Restarting module...");
  ESP.restart();
}

void ESPPortal::handleFileList() {
  DEBUG_PRINTLN("Retrieving the file system listing.");
  
  String output = "<html><head><title>LittleFS Files</title><style>body{font-family:sans-serif;padding:20px;} table{width:100%;max-width:500px;border-collapse:collapse;} th,td{padding:8px;text-align:left;border-bottom:1px solid #ddd;}</style></head><body>";
  output += "<h2>LittleFS File System Contents</h2>";
  output += "<table><tr><th>File Name</th><th>Size (Bytes)</th></tr>";

  Dir dir = LittleFS.openDir("/");

  while (dir.next()) {
    output += "<tr><td>" + dir.fileName() + "</td><td>" + String(dir.fileSize()) + "</td></tr>";
  }
  
  output += "</table>";
  output += "<br><a href='/'>&larr; Back to Setup</a> | <a href='/reset' style='color:red;'>Factory Reset ESP</a>";
  output += "</body></html>";
  
  server.send(200, "text/html", output);
}

void ESPPortal::handleHttpReset() {
  server.send(200, "text/html", "<h1>Resetting...</h1><p>The configuration file has been deleted. Reconnecting to configuration AP...</p>");
  delay(2000);
  clearCredentialsAndReset();
}

bool ESPPortal::loadCredentials() {

  DEBUG_PRINTLN((bool) LittleFS.exists(CONFIG_FILE));
  
  if (!LittleFS.exists(CONFIG_FILE)) {
    DEBUG_PRINTLN("LittleFS Config MIA");
    return false;
  }

  File configFile = LittleFS.open(CONFIG_FILE, "r");  
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    DEBUG_PRINT("Failed to parse JSON: ");
    DEBUG_PRINTLN(error.c_str());

    return false;
  }

  sta_ssid = strdup(doc["sta_ssid"]);
  sta_pass = strdup(doc["sta_pass"]);

  DEBUG_PRINT("Obtained stored SSID: ");
  DEBUG_PRINTLN(sta_ssid);
  
  isSetup = 1;

  return (strlen(sta_ssid) > 0);
}

void ESPPortal::readTextFile() {
  if (!LittleFS.begin()) {
    DEBUG_PRINTLN("An error occurred while mounting LittleFS");
    return;
  }

  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) {
    DEBUG_PRINTLN("Failed to open file for reading");
    return;
  }

  String fileContent = file.readString();
  file.close();

  server.send(200, "text/plain", fileContent);
}

wl_status_t ESPPortal::getWiFiStatus() {

  return WiFi.status();
}

void ESPPortal::handleSave() {
  DEBUG_PRINTLN("Handle save");

  if (server.hasArg("ssid")) {
    const char *wifi_ssid = server.arg("ssid").c_str();
    const char *wifi_pass = server.arg("password").c_str();

    sta_ssid = const_cast<char*>(wifi_ssid);
    sta_pass = const_cast<char*>(wifi_pass);

    DEBUG_PRINTLN("\n--- Credentials Received ---");
    DEBUG_PRINT("SSID: "); 
    DEBUG_PRINTLN(wifi_ssid);
    DEBUG_PRINT("Password: "); 
    DEBUG_PRINTLN(wifi_pass);

    const String s = sta_ssid;
    const String response = "<h1>Success!</h1><p>ESP12 is now attempting to connect to " + s + "...</p>";
    server.send(200, "text/html", response);
    
    delay(2000);

    // WiFi.onStationModeConnected(std::function<void (const WiFiEventStationModeConnected &)>)
    WiFiMode_t currentMode = WIFI_STA;
    WIFIconnect(currentMode, true);

    if (WiFi.status() == WL_CONNECTED) {
      
      saveCredentials(wifi_ssid, wifi_pass);

      DEBUG_PRINTLN("");
      DEBUG_PRINTLN("\nConnected!");
      DEBUG_PRINT("IP: "); 
      DEBUG_PRINTLN(WiFi.localIP());

      dnsServer.stop();
      
      server.begin();
      
      return; 

    } else {
      DEBUG_PRINTLN("We didn't get a connection!!!");
    }

  } else {
    server.send(400, "text/plain", "Bad Request: Missing SSID");
  }

}
#include "ESPPortal.h"
#include <iostream>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <string>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

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

const char* CONFIG_FILE = "wifi_config.json";

// Object Declarations
IPAddress apID(192, 168, 4, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);

ESPPortal::ESPPortal() {
  Serial.println("Created New Captive Portal\n");
}

void ESPPortal::handleRoot() {
  Serial.println("Handle root");
  server.send_P(200, "text/html", LOGIN_FORM);
}

bool ESPPortal::hasCredentials() {
  Serial.println("File check"); // If the file exists, we may have creds.

  File configFile = LittleFS.open(CONFIG_FILE, "r");  
  if (!configFile) {
    Serial.println(" - >> Does not have credentails .......");
    Serial.println(configFile ? true : false);
  }

  return configFile ? true : false;
}

void ESPPortal::saveCredentials(const char* ssid, const char* pass) {
  Serial.println("saveCredentials ");
  Serial.print(ssid);
  Serial.print(" " );
  Serial.print(pass);

  File file = LittleFS.open(CONFIG_FILE, "w");

  if (!file) {
    Serial.println("No file");
    return; 
  }
  Serial.println("File exists.");

  doc["sta_ssid"] = ssid;
  doc["sta_pass"] = pass;
  doc["sta_secu"] = "";
  serializeJson(doc, file);
  file.flush();
  file.close(); 

  Serial.println("[FS] Credentials saved to flash!");
}

void ESPPortal::handleNotFound() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  server.send(302, "text/plain", "Redirecting to captive portal");
}

void ESPPortal::clearCredsReset() {
  Serial.println("[RESET] Erasing Wi-Fi configurations...");

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
    Serial.print(".");
    attempts++;
  }

  if (mapRoutes) {
    server.on("/",          HTTP_GET, [this]() { this->handleRoot();        } );
    server.on("/list",      HTTP_GET, [this]() { this->handleFileList();    } );
    server.on("/list/file", HTTP_GET, [this]() { this->readTextFile();      } );
    server.on("/reset",     HTTP_GET, [this]() { this->handleHttpReset();   } );
  }

  server.begin();
  Serial.println("");
  Serial.println("Server Started");
   
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("IN STA MODE, Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
}

void ESPPortal::begin() {
    Serial.begin(115200);
    
    LittleFS.begin();
    delay(20);

    Serial.println("Begin");
      
    if (hasCredentials() == false) {
      Serial.println("\nInitializing Scoreboard Captive Portal...");

      // Access Point
      WiFi.mode(WIFI_AP);
      WiFi.softAP(AP_SSID);

      delay(10);
      
      IPAddress apIP = WiFi.softAPIP();
      Serial.print("Access Point Created. SSID: "); 
      Serial.println(AP_SSID);

      Serial.print("IP Address: "); 
      Serial.println(apIP);

      Serial.print("DNS Port ");
      Serial.println(DNS_PORT);

      // Override DNS queries (*) to the ESP IP
      dnsServer.start(DNS_PORT, "*", apIP);
      Serial.println("DNS Sever started"); 

      server.on("/",      HTTP_GET,   [this]() { this->handleRoot(); } );
      Serial.println("Route: Root registered");
      server.on("/save",  HTTP_POST,  [this]() { this->handleSave(); } );
      Serial.println("Route: Save registered");

      server.onNotFound([this]() { this->handleNotFound(); } );
      Serial.println("Route: Not Found registered");

      Serial.println("Starting server");
      server.begin();
      Serial.println("Server Started");

      Serial.println("Preparing AP");

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
        Serial.println("Did not parse file.");
        return;
      }

      Serial.print("Connecting using stored creds ");
      Serial.println(sta_ssid);
      Serial.println(sta_pass);

      WiFiMode_t currentMode = WIFI_STA;
      WIFIconnect(currentMode, true);
    }

    while(isSetup == 1 && WiFi.status() == WL_CONNECTED) {

      server.handleClient();
      delay(30);
    }

}

void ESPPortal::clearCredentialsAndReset() {
  Serial.println("[RESET] Erasing Wi-Fi configurations...");

  if (LittleFS.exists(CONFIG_FILE)) {
    Serial.println("Config file removed.");
    LittleFS.remove(CONFIG_FILE);
  }

  if (LittleFS.exists("wifi_cred.txt")) {
    Serial.println("Config file removed.");
    LittleFS.remove("wifi_cred.txt");
  }

  WiFi.disconnect(true); // Erase SDK cached credentials as well
  delay(1000);
  
  Serial.println("[RESET] Restarting module...");
  ESP.restart();
}

void ESPPortal::handleFileList() {
  Serial.println("Retrieving the file system listing.");
  
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

  Serial.println((bool) LittleFS.exists(CONFIG_FILE));
  
  if (!LittleFS.exists(CONFIG_FILE)) {
    Serial.println("LittleFS Config MIA");
    return false;
  }

  File configFile = LittleFS.open(CONFIG_FILE, "r");  
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());

    return false;
  }

  sta_ssid = strdup(doc["sta_ssid"]);
  sta_pass = strdup(doc["sta_pass"]);

  Serial.print("Obtained stored SSID: ");
  Serial.println(sta_ssid);
  
  isSetup = 1;

  return (strlen(sta_ssid) > 0);
}

void ESPPortal::readTextFile() {
  if (!LittleFS.begin()) {
    Serial.println("An error occurred while mounting LittleFS");
    return;
  }

  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
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
  Serial.println("Handle save");

  if (server.hasArg("ssid")) {
    const char *wifi_ssid = server.arg("ssid").c_str();
    const char *wifi_pass = server.arg("password").c_str();

    sta_ssid = const_cast<char*>(wifi_ssid);
    sta_pass = const_cast<char*>(wifi_pass);

    Serial.println("\n--- Credentials Received ---");
    Serial.print("SSID: "); 
    Serial.println(wifi_ssid);
    Serial.print("Password: "); 
    Serial.println(wifi_pass);

    const String s = sta_ssid;
    const String response = "<h1>Success!</h1><p>ESP12 is now attempting to connect to " + s + "...</p>";
    server.send(200, "text/html", response);
    
    delay(2000);

    // WiFi.onStationModeConnected(std::function<void (const WiFiEventStationModeConnected &)>)
    WiFiMode_t currentMode = WIFI_STA;
    WIFIconnect(currentMode, true);

    if (WiFi.status() == WL_CONNECTED) {
      
      saveCredentials(wifi_ssid, wifi_pass);

      Serial.println("");
      Serial.println("\nConnected!");
      Serial.print("IP: "); 
      Serial.println(WiFi.localIP());

      dnsServer.stop();
      
      server.begin();
      
      return; 

    } else {
      Serial.println("We didn't get a connection!!!");
    }

  } else {
    server.send(400, "text/plain", "Bad Request: Missing SSID");
  }

}
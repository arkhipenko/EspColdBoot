/*
   ColdBoot ROM for ESP8266
   Copyright (c) 2020, Anatoli Arkhipenko
   All rights reserved.

*/


#define APPVER    "1.0.0"
#define APPTOKEN  "COLDBOOT"
#define EEPROM_MAX 4096
#define _JSONCONFIG_NOSTATIC
#define APPPREFIX "coldboot-"
//#define APPPREFIX "success-"

#define _DEBUG_

#ifdef _DEBUG_
#define _PP(a) Serial.print(a);
#define _PL(a) Serial.println(a);
#else
#define _PP(a)
#define _PL(a)
#endif

// ==== Includes ===================================

#include <EspBootstrapDict.h>
#include <JsonConfigHttp.h>
#include <JsonConfigSPIFFS.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WiFi.h>

// ==== Globals ===========================
String appVersion;

// ==== CODE ==============================
void printDictionary(Dictionary& d, String line) {
  _PL(line);
  for (int i = 0; i < d.count(); i++) {
    _PP('\t'); _PP(d(i)); _PP("\t:\t"); _PL(d[i]);
  }
  _PP("\tCurrent count = "); _PL(d.count());
  _PP("\tCurrent size  = "); _PL(d.size());
  _PL();
}

void setup(void) {
  bool wifiTimeout = true;

  // Debug console
#ifdef _DEBUG_
  Serial.begin(115200);
  delay(3000);
  {
    _PL("ESP8266 ColdBoot v1.0.0"); _PL();
    String id = "ESP8266-" + String(ESP.getChipId(), HEX);
    _PP("ESP8266 ID: "); _PL(id);
  }
#endif

  setupOTA();
  _PP("Full Application Version: "); _PL(appVersion);
  _PL("---------------------------------------------");

  //  First let's create a dictionary
  Dictionary* ptr_d = new Dictionary(5);
  if ( !ptr_d ) stopOnError("E: Error creating Dictionary");
  Dictionary& d = *ptr_d;

  d("Title", "ColdBoot Initial Config");
  d("SSID", "<your wifi ssid>");
  d("Password", "<your wifi password>");
  d("Config", "<path to configuration>");
  d("OTA", "<path to OTA firmware>");
  d("fields", "5");

  //  printDictionary(d, "Initial dictionary:");

  //  Check if we have a filesystem and try reading 'config.json' file from it
  SPIFFSConfig cfg;
  cfg.setAutoFormat(false);
  SPIFFS.setConfig(cfg);
  SPIFFS.begin();

  _PL();
  _PL("STEP 1:");
  _PL(" > Reading configuration file from SPIFFS if present");

  JsonConfigSPIFFS* js = new JsonConfigSPIFFS;
  if ( !js ) stopOnError("E: Error creating JsonConfigSPIFFS");

  if ( js->parse("/config.json", d) == JSON_OK ) {
    //  Succeeded reading configuration file from the SPIFFS
    //  Try connecting to WiFi to get OTA update

    _PL(" > SUCCESS.");
    _PL();
    _PL("STEP 2:");
    printDictionary(d, " > Configuration read from SPIFFS:");

    _PP(" > Attempting WiFi connection ");
    setupWifi(d["SSID"].c_str(), d["Password"].c_str());
    wifiTimeout = waitForWifi(30 * BOOTSTRAP_SECOND);
    if ( wifiTimeout ) {
      _PL(" > WiFi connection unsuccessful.");
      _PL();
    }
  }
  else {
    _PL(" > A valid cofiguration file could not be read from SPIFFS");
    _PL(" > Skipping STEP 2");
  }
  delete js;

  //  At this point if we are connected to WiFi then the configuration
  //  file on SPIFFS was good, and we can attempt OTA.
  //  Otherwize, we need BootStrapping

  while ( wifiTimeout ) {
    //  We are going to Bootstrap the device until it successfully connects to WiFi
    _PL();
    _PL("STEP 3:");
    _PL(" > Provisioning configuration from the user");
    _PL();
    _PL("\tNavigate to  http://10.1.1.1  and fill out the form");
    _PL("\tProvide WiFi SSID and password");
    _PL("\tas well as URLs for Configuration and/or OTA");

    int fields = d["fields"].toInt();
    if ( fields <= 0 || fields > 5 ) fields = 5;
    if ( ESPBootstrap.run(d, fields) == BOOTSTRAP_OK ) {
      _PL(" > Bootstrapping SUCCESS.");
      printDictionary(d, " > Configuration after Bootstrap:");

      _PP(" > Attempting WiFi connection ");
      setupWifi(d["SSID"].c_str(), d["Password"].c_str());
      wifiTimeout = waitForWifi(30 * BOOTSTRAP_SECOND);
      if ( wifiTimeout ) {
        _PL(" > WiFi connection unsuccessful.");
      }
    }
    else {
      _PL(" > Bootstrapping UNSUCCESSFUL.");
      _PL(" > Re-trying...");
      _PL();
    }
  }

  //  At this point we should be connected to WiFi and can attempt either OTA
  //  or reading configuration from HTTP server

  _PL();
  _PL("STEP 4:");
  _PL(" > Checking if configuration should be read from an HTTP server");

  if ( d["Config"].startsWith("http") ) {
    //  Looks like we might have a proper Config URL!
    //  Lets construct a config name and attempt reading it.
    //  The name will be http://your-config-server/coldboot-deviceid-version.json
    //  E.g.:  http://ota.home.lan/esp/config/coldboot-daa238-1.0.0.json

    String url = makeConfig(d["Config"]);
    _PL(" > Attempting to read config from this URL:");
    _PP('\t'); _PL(url);

    JsonConfigHttp* js = new JsonConfigHttp;
    if ( !js ) stopOnError("E: Error creating JsonConfigHttp");

    if ( js->parse( url, d ) == JSON_OK ) {
      _PL(" > SUCCESS.");
      printDictionary( d, " > Configuration after HTTP config:" );
    }
    delete js;
  }
  else {
    _PL(" > URL provided does not contain a valid configuration");
  }

  _PL();
  _PL("STEP 5:");
  _PL(" > Checking for the OTA provisioning URL");

  //  If successful we should finally have the OTA URL defined and can attempt an OTA
  //  update. If not, we need to Reboot and start all over.
  //  The URL will be http://your-ota-server/coldboot-deviceid-version.bin
  //  E.g.:  http://ota.home.lan/esp/bin/coldboot-daa238-1.0.0.bin
  if ( d["OTA"].startsWith("http") ) {
    //  Looks like we might have a proper OTA URL!
    //  Lets try it.
    String url = makeOTA(d["OTA"]);
    _PL(" > Attempting OTA Update from this URL:");
    _PP('\t');
    _PL( url );

    checkOTA( url );
  }
  _PL();
  _PL("STEP 6:");
  _PL(" > Unable to get a valid OTA update. Will reboot in 10 seconds");
  delay(10000);
  ESP.restart();
}

void loop(void) {}


String makeConfig(String path) {
  String cfg(path);

  if ( !cfg.endsWith(".json")) {
    if (!cfg.endsWith("/")) cfg += "/";
    cfg += (appVersion + ".json");
  }
  return cfg;
}

String makeOTA(String path) {
  String cfg(path);

  if (!cfg.endsWith(".bin")) {
    if (!cfg.endsWith("/")) cfg += "/";
    cfg += (appVersion + ".bin");
  }
  return cfg;
}

// This method prepares for WiFi connection
void setupWifi(const char* ssid, const char* pwd) {

  // We start by connecting to a WiFi network
  // clear wifi config
  WiFi.disconnect();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);
}


// This method waits for a WiFi connection for aTimeout milliseconds.
bool waitForWifi(unsigned long aTimeout) {

  unsigned long timeNow = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    _PP(".");
    if (millis() - timeNow > aTimeout) {
      _PL(" WiFi connection timeout");
      return true;
    }
  }

  _PL("\tWiFi connected");
  _PP("\tIP address: "); _PL(WiFi.localIP());
  _PP("\tSSID: "); _PL(WiFi.SSID());
  _PP("\tmac: "); _PL(WiFi.macAddress());
  delay(2000); // let things settle
  return false;
}

void setupOTA() {
  pinMode(LED_BUILTIN, OUTPUT);
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

  //  ESPhttpUpdate.onStart(update_started);
  //  ESPhttpUpdate.onEnd(update_finished);
  //  ESPhttpUpdate.onProgress(update_progress);
  //  ESPhttpUpdate.onError(update_error);
  appVersion = String(APPPREFIX) + String(ESP.getChipId(), HEX) + String("-") + String(APPVER);
  appVersion.toLowerCase();
}

void checkOTA(String aUrl) {
  WiFiClient espClient;

  t_httpUpdate_return ret = ESPhttpUpdate.update( espClient, aUrl );
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      _PP("HTTP_UPDATE_FAILED: Error code=");
      _PP(ESPhttpUpdate.getLastError());
      _PP(" ");
      _PL(ESPhttpUpdate.getLastErrorString());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      _PL("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      _PL("HTTP_UPDATE_OK");
      break;
  }
}

void stopOnError(String aStr) {
  _PL(aStr);
  delay(5000);
  ESP.restart();
}

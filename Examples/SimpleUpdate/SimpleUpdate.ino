#include <Arduino.h>

#include <MCTimer.h>
#if defined(ESP32)
  #include <WiFi.h>
  #include <ESPmDNS.h>
  #include <WebServer.h>
  #include <esp_wifi.h> //  for setting power
  WebServer server(80);
  typedef system_event_id_t MCWiFiEvent;
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WifiUDP.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266WebServer.h>
  ESP8266WebServer server(80);
  typedef WiFiEvent_t MCWiFiEvent;
#endif
#include <MCOTAUpdater.h>

#define HOSTNAME      "BOARD_HOSTNAME"
#define SSID          "YOUR_SSID"
#define SSID_PASSWORD "YOUR_SSID_PASSWORD"


// you must copy in this Website path your SimpleUpdate.bin and 
// You must create and copy a json file named your SimpleUpdate.bin.version.json
// in that file you have to write your SimpleUpdate.bin version like following row:
// {"v1":1,"v2":0,"v3":0,"v4":0}

String OTAUpdateServer = "Your OTA WEB Server";
String OTAUpdateUri    = "/esp/SimpleUpdate/SimpleUpdate.bin";  // your uri
MCOTAUpdater MCOtaUpdate(OTAUpdateServer, OTAUpdateUri, 1,0,0,0);


void updateUTC_Clock(uint8_t timezone = 0) {
  configTime(timezone, 0, "pool.ntp.org", "time.nist.gov");  // UTC

  Serial.print(F("Waiting for NTP time sync: "));
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    yield();
    delay(500);
    Serial.print(F("."));
    now = time(nullptr);
  }
  Serial.println(F(""));
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}

void WiFiEvents(MCWiFiEvent event)
{
  Serial.printf("Event: %d\n", (int)event);
#if defined(ESP8266) 
  if (event == MCWiFiEvent::WIFI_EVENT_SOFTAPMODE_STACONNECTED)
#elif defined(ESP32)
  if (event == MCWiFiEvent::SYSTEM_EVENT_STA_CONNECTED)
#endif
  {
    Serial.println('\n');
    Serial.println(F("###############################"));
    Serial.println("Connection established!");  
    Serial.println(F("###############################"));
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP()); 
    Serial.print("MAC address:\t");
    Serial.println(WiFi.macAddress()); 


    updateUTC_Clock();
    Serial.println(F("###############################"));

    server.begin();

    server.on("/", [](){
      time_t rawtime;
      struct tm* timeinfo;
      char buffer[80];

      time(&rawtime);
      timeinfo = localtime(&rawtime);

      strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
      server.send(200,"text/html", "Hello World<br/> version= " + MCOtaUpdate.GetCurrentVersion().toString()  + 
                      "<br>Date Time: " + String(buffer));
    });
  }

}


void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("WAIT %d...\n", t);
    Serial.flush();
    delay(500);
  }
  Serial.print("Version: " );
  Serial.println(MCOtaUpdate.GetCurrentVersion().toString());


  int8_t power = -128;
  esp_wifi_set_max_tx_power( power  );
  esp_wifi_get_max_tx_power(&power);
  Serial.printf("power %d\n", (int) power);

  #if defined(ESP8266)  
    WiFi.onEvent(WiFiEvents, WiFiEvent_t::WIFI_EVENT_SOFTAPMODE_STACONNECTED);
  #elif defined(ESP32)
    WiFi.onEvent(WiFiEvents, system_event_id_t::SYSTEM_EVENT_STA_CONNECTED);
  #endif

  WiFi.begin(SSID, SSID_PASSWORD);
  WiFi.setAutoReconnect(true);
  while(WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(F("."));
    yield();
  }
}

void loop() {
  
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) 
  {
    server.handleClient();
    static MCOTAUpdater::Version LastVersionFound;
    MCOTAUpdater::Version VersionFound =  MCOtaUpdate.Handle(20); // each 20  Seconds
    if (VersionFound.status!=LastVersionFound.status)
    {
      LastVersionFound = VersionFound ;
      switch (VersionFound.status) {
        case MCOTAUpdater::Error::MCOTA_UPDATE_FAILED:
            Serial.println ( "MCOTA_UPDATE_FAILED Error");
            break;
        case MCOTAUpdater::Error::MCOTA_UPDATE_NO_UPDATES:
            Serial.println("MCOTA_UPDATE_NO_UPDATES available!");
            break;
        case MCOTAUpdater::Error::MCOTA_UPDATE_NO_VALID_URI:
            Serial.println("MCOTA_UPDATE_NO_VALID_URI Error");
            break;
        case MCOTAUpdater::Error::MCOTA_UPDATE_NO_WEB_SITE:
            Serial.println("MCOTA_UPDATE_NO_VALID_URI Error");
            break;
        case MCOTAUpdater::Error::MCOTA_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            break;
      }
    }
    if (MCOtaUpdate.ExistNewVersion(20)) // check for update each 20 seconds (default is 2 hours)
    {
      Serial.print("New Version Found: " );
      Serial.println(VersionFound.toString());
      Serial.println("Updating....");
      MCOtaUpdate.Update(2, HIGH);
    }
  }
  
}

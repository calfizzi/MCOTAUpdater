#include "MCOTAUpdater.h"
#include <WiFiClient.h>

#if defined(ESP32)
  #include <HTTPUpdate.h>
  #define HTTPUpdate httpUpdate
#elif defined (ESP8266)
  #include <ESP8266httpUpdate.h>
  #define HTTPUpdate ESPhttpUpdate
#endif

void inline _MyDEBUG(const char *functionName, const char *Name)
{
  Serial.print (functionName);  
  Serial.print ("::"); 
  Serial.print (Name);  
  Serial.print ("= "); 
}

#define DEBUG 0
#define LOG(value) if (DEBUG==1){ _MyDEBUG(__func__, #value); Serial.println(value);}


MCOTAUpdater::MCOTAUpdater(String Hostname, String Uri, byte v1, byte v2, byte v3, byte v4)
{
  _init(Hostname, Uri);
  SetCurrentVersion(v1, v2, v3, v4);
}

void                  MCOTAUpdater::_init              ( String Hostname, String Uri)
{
  this->_Hostname = Hostname;
  this->_Uri      = Uri;
  this->_NewVersion.Clear();
  this->_CurrentVersion.Clear();
}
void                  MCOTAUpdater::SetCurrentVersion  ( byte v1, byte v2, byte v3, byte v4)
{
  _CurrentVersion.Clear();
  _CurrentVersion.ver1 = v1;
  _CurrentVersion.ver2 = v2;
  _CurrentVersion.ver3 = v3;
  _CurrentVersion.ver4 = v4;
}
bool                  MCOTAUpdater::ExistNewVersion    ( uint16_t intervalInSeconds )
{
  Handle(intervalInSeconds);
  return this->_NewVersion>this->_CurrentVersion;
}
MCOTAUpdater::Version &MCOTAUpdater::GetNewVersion     ( void ) {return this->_NewVersion;}
MCOTAUpdater::Version &MCOTAUpdater::GetCurrentVersion ( void ) {return this->_CurrentVersion;}
String                MCOTAUpdater::_FindJsonKey       ( String Data, String SearchFor)
{
  String SearchString ="\"" + SearchFor + "\":";
  int index = Data.lastIndexOf(SearchString);
  if(index>=0)
  { 
    int lastIndex = Data.indexOf(',', index);
    if (lastIndex <0)
      lastIndex = Data.indexOf('}', index);
    String ReturnData = Data.substring(index + SearchString.length(), lastIndex);
    LOG( ReturnData );
    return ReturnData ;
  }
  else 
    return "";
}
MCOTAUpdater::Version &MCOTAUpdater::Handle            ( uint16_t intervalInSeconds)
{
  static uint32_t ms = 0;
  uint32_t interval = (uint32_t)1000 * (uint32_t)intervalInSeconds;

  if ( ms==0 || millis()-ms> interval  || 
     ( ms>millis() &&  (0XFFFFFFFFLL - ms) + millis()>interval)) 
  {
    ms = millis();
    String host = this->_Hostname ;
    String URI = this->_Uri + ".version.json";
  
    WiFiClient client;
    uint32_t ms = millis();
    LOG("ExistOTAUpdate...");
    while (!!!client.connect(host.c_str(), 80) && (millis()- ms) < 2000) {
      yield();
    }
    if ((millis()- ms) >= 2000)
    {
      LOG("connection failed!");
      this->_NewVersion.status = Error::MCOTA_UPDATE_NO_WEB_SITE;
      return this->_NewVersion;
    }
    client.write(("GET " + URI + " HTTP/1.1\r\n"  +
                  "Host: " + host + "\r\n" +
                  "Accept: text/html,application/json\r\n"
                  //"Accept: text/html\r\n"
                  //"Accept: text/plain\r\n" +
                  "Accept-Encoding: gizip, deflate\r\n" +
                  "User-Agent: ESP32\r\n"
                  "Connection: keep-alive\r\n\r\n").c_str());
    while(!client.available()) {yield();}
    if (!client.available())
      this->_NewVersion.status = Error::MCOTA_UPDATE_NO_VALID_URI;
    while(client.available())
    {
      String Data = client.readStringUntil('\n');
      if (DEBUG == 1)
        Serial.println(Data);
      String httpOK = this->_FindJsonKey(Data, "HTTP/1.1 200 OK");
      if (httpOK == "")
      {
        this->_NewVersion.status = Error::MCOTA_UPDATE_NO_WEB_SITE;
      }
      String v1 = this->_FindJsonKey(Data, "v1");
      String v2 = this->_FindJsonKey(Data, "v2");
      String v3 = this->_FindJsonKey(Data, "v3");
      String v4 = this->_FindJsonKey(Data, "v4");
      if ( v1 != "" && v2 != "" && v3!="" && v4!="" )
      {
        this->_NewVersion.ver1 = (byte)v1.toInt();
        this->_NewVersion.ver2 = (byte)v2.toInt();
        this->_NewVersion.ver3 = (byte)v3.toInt();
        this->_NewVersion.ver4 = (byte)v4.toInt();

      
        if (this->_NewVersion>this->_CurrentVersion)
          this->_NewVersion.status = Error::MCOTA_UPDATE_OK ;
        else
          this->_NewVersion.status = Error::MCOTA_UPDATE_NO_UPDATES;
      }
    }
    client.stop();
  }
  return this->_NewVersion;
}
MCOTAUpdater::Error    MCOTAUpdater::Update            ( uint8_t PinLed, bool NormalStatus)
{
  Serial.println("Self Update Starting....");
  WiFiClient client;

  HTTPUpdate.setLedPin(PinLed, NormalStatus);
  //t_httpUpdate_return ret = httpUpdate.update(client, "http://192.168.0.20/esp32/test/MC_ESP_WebSession.ino.bin");
  // Or:
  if (this->ExistNewVersion())
  {
    t_httpUpdate_return ret = HTTPUpdate.update(client, this->_Hostname, 80, this->_Uri);

    switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", HTTPUpdate.getLastError(), HTTPUpdate.getLastErrorString().c_str());
      return Error::MCOTA_UPDATE_FAILED;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      return Error::MCOTA_UPDATE_NO_UPDATES;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      return Error::MCOTA_UPDATE_OK;
      break;
    }
    return Error::MCOTA_UPDATE_NO_WEB_SITE;
  }
  return Error::MCOTA_UPDATE_NO_UPDATES;
}


//MCOTAUpdate::Error    MCOTAUpdate::UpdateIfAvailable(String Hostname, String Uri, MCOTAUpdate::Version &CurrentVersion)
//{
//  //static MCTimer T(20000);
//  //if (!T.IsStarted() || T.IsElapsed())
//  //{
//    Serial.println("Check For Update....");
//    //T.Start();
//    // The line below is optional. It can be used to blink the LED on the board during flashing
//    // The LED will be on during download of one buffer of data from the network. The LED will
//    // be off during writing that buffer to flash
//    // On a good connection the LED should flash regularly. On a bad connection the LED will be
//    // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
//    // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
//    // httpUpdate.setLedPin(LED_BUILTIN, LOW);
//    this->Available(Hostname, Uri, CurrentVersion);
//    if (this->VersionFound.status == Error::MCOTA_UPDATE_OK)
//    {
//      return this->Update(Hostname, Uri);
//    }
//    return this->VersionFound.status;
//  //}
//}
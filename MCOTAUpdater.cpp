/*

  Module: MCOTAUpdater 
  Author: Mario Calfizzi (MC)

  Description:
      This Library works on ESP8266 and ESP32 to manage OTA update of your ESP board.
      copy in your Website (your path) the SimpleUpdate.bin generated
      copy a json (text format) file named your SimpleUpdate.bin.version.json
      in that file you have to write your SimpleUpdate.bin.version.json like following row:
      {"v1":1,"v2":0,"v3":0,"v4":0}

      if you want update the SPIFFS file System TOO you have to write into your "SimpleUpdate.bin.version.json" something like this:

      {"v1":1,"v2":0,"v3":0,"v4":0,"SPIFFS_format":0,"SPIFFS_update_files":["/index.html","/index2.html","/images/monkey-logo.png"]}

        "SPIFFS_format": 0 for no Format, 1 for format
        "SPIFFS_update_files": [array list] contains a list of file to update on your ESP SPIFFS file System

   Location: https://github.com/calfizzi/MCOTAUpdater

*/
#include "MCOTAUpdater.h"
#include <WiFiClient.h>
//#include <MCDebugger.h>

#if defined(ESP32)
  #include <HTTPUpdate.h>
  #include <FS.h>
  #include <SPIFFS.h>
  #define HTTPUpdate httpUpdate
#elif defined (ESP8266)
  #include <ESP8266httpUpdate.h>
  #include <FS.h>
  #define HTTPUpdate ESPhttpUpdate
#endif

#ifndef  FILE_READ
  #define  FILE_READ "r"
#endif
#ifndef  FILE_WRITE
  #define  FILE_WRITE "w"
#endif
#ifndef  FILE_APPEND
  #define  FILE_APPEND  "a"
#endif




#ifndef MCDebugger_h

#define DEBUG 0

void _MyDEBUGFunc(const char *filename, const char *functionName, int line)
{
    Serial.printf ("%05d: %s", line,functionName);  
}

void _MyDEBUG(const char *filename, const char *functionName, int line, const char *Name)
{
  _MyDEBUGFunc ( filename, functionName, line);
  Serial.print ("::"); 
  Serial.print (Name);  
  Serial.print ("= "); 
}

#define LOG(value) if (DEBUG==1){ _MyDEBUG(__FILE__, __func__,__LINE__, #value); Serial.println(value);}
#define LOGMSG(description) {if (DEBUG!=0){_MyDEBUGFunc(__FILE__, __func__, __LINE__);Serial.print("--> "); Serial.println(description);}}
#define LOGFUNC(description) {if (DEBUG!=0){_MyDEBUGFunc(__FILE__, __func__, __LINE__);Serial.println();}}
#endif

MCOTAUpdater::MCOTAUpdater(String Hostname, String Uri, uint16_t port, byte v1, byte v2, byte v3, byte v4)
{
  this->Begin(Hostname, Uri, port, v1, v2, v3, v4);
}
void MCOTAUpdater::Begin(String Hostname, String Uri, uint16_t port, byte v1, byte v2, byte v3, byte v4)
{
  _init(Hostname, Uri, port);
  SetCurrentVersion(v1, v2, v3, v4);
}

void                    MCOTAUpdater::_init                     ( String Hostname, String Uri, uint16_t port)
{
  this->_Hostname = Hostname;
  this->_Uri      = Uri;
  this->_port     = port;
  this->_NewVersion.Clear();
  this->_CurrentVersion.Clear();
}
void                    MCOTAUpdater::SetCurrentVersion         ( byte v1, byte v2, byte v3, byte v4)
{
  _CurrentVersion.Clear();
  _CurrentVersion.ver1 = v1;
  _CurrentVersion.ver2 = v2;
  _CurrentVersion.ver3 = v3;
  _CurrentVersion.ver4 = v4;
}
bool                    MCOTAUpdater::ExistNewVersion           ( uint16_t intervalInSeconds )
{
  Handle(intervalInSeconds);
  return this->_NewVersion>this->_CurrentVersion;
}
MCOTAUpdater::Version  &MCOTAUpdater::GetNewVersion             ( void ) {return this->_NewVersion;}
MCOTAUpdater::Version  &MCOTAUpdater::GetCurrentVersion         ( void ) {return this->_CurrentVersion;}
std::vector<String>     MCOTAUpdater::_JsonGetArray             ( String Data)
{
  std::vector<String> returnValue;
  int startIndex = Data.indexOf('[');
  int endIndex = Data.lastIndexOf(']');
  String partialData = Data.substring(startIndex+1, endIndex);
  LOG(partialData );
  while(partialData.indexOf(',')>-1)
  { 
    endIndex = partialData.indexOf(',');
    if (endIndex< 0)
      endIndex = partialData.length();

    String str = partialData.substring(0, endIndex);
    returnValue.push_back(str);
    partialData = partialData.substring(endIndex+1, partialData.length());
  }
  if (partialData.length()>0)
    returnValue.push_back(partialData);
  return returnValue;
}
String                  MCOTAUpdater::_FindJsonKey              ( String Data, String SearchFor)
{
  String SearchString ="\"" + SearchFor + "\":";
  int index = Data.lastIndexOf(SearchString);
  if(index>=0)
  { 
    int endIndex = 0 ;
    if (Data.c_str()[index + SearchString.length()] == '\"'  )
      endIndex = Data.indexOf(',', index);
    else if (Data.c_str()[index + SearchString.length()] == '{'  )
      endIndex = Data.indexOf('}', index + SearchString.length())+1;
    else if (Data.c_str()[index + SearchString.length()] == '['  )
      endIndex = Data.indexOf(']', index + SearchString.length())+1;
    else
      endIndex = Data.indexOf(',', index);

    if (endIndex <0)
      endIndex = Data.indexOf('}', index);
    String ReturnData = Data.substring(index + SearchString.length(), endIndex);
    LOG( ReturnData );
    return ReturnData ;
  }
  else 
    return "";
}
bool                    MCOTAUpdater::_httpGetFileToSPIFFSSave  ( String fileUri)
{
  uint32_t ms = millis();
  String host = this->_Hostname ;
  String URI  = this->_Uri.substring(0, this->_Uri.lastIndexOf("/")) + fileUri;
  WiFiClient client;
  bool returnValue = true;
  IPAddress   HostIp;
  WiFi.hostByName ( host.c_str(), HostIp);
  
  LOG(HostIp);
  LOG(URI);
  
#if defined(ESP32)
  while (!!!client.connect(HostIp, this->_port, 5) && (millis()- ms) < 5000) {
#elif defined(ESP8266)
  client.setTimeout(5000);
  while (!!!client.connect(HostIp, this->_port) && (millis()- ms) < 5000) {
#endif
    yield();
  }
  if ((millis()- ms) >= 5000)
  {
    LOGMSG("connection failed!");
    Serial.println("Connection failed!");
    this->_NewVersion.status = Error::MCOTA_UPDATE_NO_WEB_SITE;
    return false;
  }
  client.write(("GET " + URI + " HTTP/1.1\r\n"  +
                "Host: " + host + "\r\n" +
                //"Accept: text/html,application/json\r\n"
                //"Accept: text/html\r\n"
                //"Accept: text/plain\r\n" +
                "Accept-Encoding: gizip, deflate\r\n" +
                "User-Agent: ESP32\r\n"
                "Connection: keep-alive\r\n\r\n").c_str());
  while(!client.available()) {yield();}
  if (!client.available())
    this->_NewVersion.status = Error::MCOTA_UPDATE_NO_VALID_URI;

  pinMode (this->_FlashPin, OUTPUT);

  size_t fileSize = 0;
  if (client.available())
  {
    String s = client.readStringUntil('\n');
    s.toUpperCase();
    LOG(s)
    if (s.indexOf("HTTP/1.1 200 OK") == 0)
    {
      String Size_String = "Content-Length:";
      while (client.available())
      {
        String CurrRow = client.readStringUntil('\n') + "\n";

        if (CurrRow.indexOf(Size_String)==0)
        {
          fileSize = atol(CurrRow.substring(Size_String.length()).c_str());
          LOG(CurrRow.substring(Size_String.length()));
          LOG(fileSize );

        }

        //Serial.print(CurrRow);
        if (CurrRow == "\n" || CurrRow == "\r\n")
          break;
      }
    }
  }
  if(client.available())
  {
    #if defined(ESP32)
        SPIFFS.begin(true);
    #elif defined(ESP8266)
        SPIFFS.begin();
    #endif
    if (SPIFFS.remove(fileUri)==0)
      Serial.printf("Cannot remove file: %s\n", fileUri.c_str());
    File uploadFile = SPIFFS.open(fileUri, FILE_WRITE);
    if (!uploadFile)
    {
      Serial.printf("Cannot Open file: %s, error %d\n", fileUri.c_str(), uploadFile.getWriteError());
      client.stop();
      return false;
    }
    Serial.printf("Updating SPIFFS: %s\n", fileUri.c_str());
    bool light = false;
    size_t count = 0;
    //Serial.printf("fileSize= %d\n", fileSize);
    while( count<fileSize)
    {
      if (client.available())
      {
        byte b = client.read();
        if (uploadFile.write(b) == 0)
        {
          Serial.printf("Cannot write file: %s, error %d\n", fileUri.c_str(), uploadFile.getWriteError());
          returnValue  = false;
          break;
        }

        if (millis() % 64 == 0) light = !light;
        digitalWrite(this->_FlashPin, light);
        count++;
        //Serial.print((char)b);
      }
      
    }
    uploadFile.close();
  }
  client.stop();
  return returnValue ;
}
MCOTAUpdater::Version  &MCOTAUpdater::Handle              ( uint16_t intervalInSeconds)
{
  static uint32_t ms = 0;
  uint32_t interval = (uint32_t)1000 * (uint32_t)intervalInSeconds; // transform to millis
  if (WiFi.status()!=wl_status_t::WL_CONNECTED) return this->_NewVersion;

  if ( ms==0 || millis()-ms> interval  || 
     ( ms>millis() &&  (0XFFFFFFFFLL - ms) + millis()>interval)) 
  {
    //Serial.println("MCOTAUpdater::Handle");
    LOGFUNC();
    ms = millis();
    String host = this->_Hostname ;
    String URI = this->_Uri + ".version.json";
  
    WiFiClient client;
    uint32_t ms = millis();
    LOGMSG("ExistOTAUpdate...");
    LOG(host);
    LOG(this->_port);

    IPAddress   HostIp;
    WiFi.hostByName ( host.c_str(), HostIp);
    
    LOG(HostIp);
    LOG(URI);
#if defined(ESP32)
  while (!!!client.connect(HostIp, this->_port, 5) && (millis()- ms) < 5000) {
#elif defined(ESP8266)
  client.setTimeout(5000);
  while (!!!client.connect(HostIp, this->_port) && (millis()- ms) < 5000) {
#endif
      yield();
    }
    if ((millis()- ms) >= 5000)
    {
      LOGMSG("connection failed!");
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
      String v1     = this->_FindJsonKey(Data, "v1");
      String v2     = this->_FindJsonKey(Data, "v2");
      String v3     = this->_FindJsonKey(Data, "v3");
      String v4     = this->_FindJsonKey(Data, "v4");
      String Files  = this->_FindJsonKey(Data, "SPIFFS_update_files");
      String frmt   = this->_FindJsonKey(Data, "SPIFFS_format");
      this->_FormatSPIFFS = frmt.toInt() == 1;
      std::vector<String> list = this->_JsonGetArray(Files);
      LOG ( Files )
      if ( v1 != "" && v2 != "" && v3!="" && v4!="" )
      {
        this->_NewVersion.ver1 = (byte)v1.toInt();
        this->_NewVersion.ver2 = (byte)v2.toInt();
        this->_NewVersion.ver3 = (byte)v3.toInt();
        this->_NewVersion.ver4 = (byte)v4.toInt();
        
        this->_FileList.clear();
        for (size_t i = 0; i < list.size(); i++)
        {
          LOG ( list[i] );
          list[i].replace("\"","");
          this->_FileList.push_back(list[i]);
        }

      
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
void                    MCOTAUpdater::SetPinStatus        ( uint8_t FlashPinLed, bool FlashPinNormalStatus)
{
  this->_FlashPin             = FlashPinLed;
  this->_FlashPinNormalStatus = FlashPinNormalStatus;
}
MCOTAUpdater::Error     MCOTAUpdater::Update              ( void )
{
  return this->Update(this->_FlashPin, this->_FlashPinNormalStatus);
}
MCOTAUpdater::Error     MCOTAUpdater::Update              ( uint8_t FlashPinLed, bool FlashPinNormalStatus)
{
  WiFiClient client;
  this->_FlashPin = FlashPinLed;
  Error returnValue = Error::MCOTA_UPDATE_OK;

  HTTPUpdate.setLedPin(FlashPinLed, FlashPinNormalStatus);
  //t_httpUpdate_return ret = httpUpdate.update(client, "http://192.168.0.20/esp32/test/MC_ESP_WebSession.ino.bin");
  // Or:
  if (this->ExistNewVersion())
  {
    //returnValue = Error::MCOTA_UPDATE_NO_WEB_SITE;
    Serial.println("Updating SPIFFS Files...");
    for (size_t i = 0; i < this->_FileList.size(); i++)
    {
      LOG ( this->_FileList[i] );
      if (!this->_httpGetFileToSPIFFSSave(this->_FileList[i]))
      {
        returnValue = Error::MCOTA_UPDATE_SPIFFS_UPDATE_FAILURE;
        Serial.println("asdasdsdsadsad*****asdasdasddas");
        break;
      }
    }
    if (returnValue == Error::MCOTA_UPDATE_OK)
    {
      Serial.println("Self Update Starting....");
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
    }
    return returnValue;
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
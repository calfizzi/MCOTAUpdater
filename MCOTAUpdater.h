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

#pragma once
#ifndef MCOTAUpdater_h
#define MCOTAUpdater_h
#include <Arduino.h>


class MCOTAUpdater
{
public:
  typedef std::vector<String> FileList;
  typedef enum Error_e
  {
    MCOTA_UPDATE_FAILED,
    MCOTA_UPDATE_NO_UPDATES,
    MCOTA_UPDATE_OK,
    MCOTA_UPDATE_NO_WEB_SITE,
    MCOTA_UPDATE_NO_VALID_URI,
    MCOTA_UPDATE_SPIFFS_UPDATE_FAILURE
  }Error;
  typedef struct Version_s
  {
    union 
    {
      byte bytes[4];
      struct
      {
        byte ver1;
        byte ver2;
        byte ver3;
        byte ver4;
      };
    };
    Error status;
    bool operator < (struct Version_s &value)  { return memcmp(bytes, value.bytes, 4)<0; }
    bool operator == (struct Version_s &value) { return memcmp(bytes, value.bytes, 4)==0;}
    bool operator > (struct Version_s &value)  { return memcmp(bytes, value.bytes, 4) >0;}
    bool operator <= (struct Version_s &value) { return memcmp(bytes, value.bytes, 4)<=0;}
    bool operator >= (struct Version_s &value) { return memcmp(bytes, value.bytes, 4)>=0;}
    String toString()
    {
      return String ( String(ver1) + "." + String(ver2) + "." + String(ver3) + "." + String(ver4) );
    }
    void Clear(){memset(this, 0, 4 + sizeof(Error));}
  }Version;
private:
  Version   _NewVersion;
  Version   _CurrentVersion;
  String    _Hostname;
  String    _Uri;
  uint16_t  _port      = 80;
  int8_t    _FlashPin = -1;
  uint8_t   _FlashPinNormalStatus = true;
  FileList  _FileList;
  bool      _FormatSPIFFS = false;

private:
  void                 _init                    ( String Hostname, String Uri, uint16_t Port = 80);
  String               _FindJsonKey             ( String Data, String SearchFor);
  std::vector<String>  _JsonGetArray            ( String Data);
  bool                 _httpGetFileToSPIFFSSave ( String Uri = "");
public:
                        MCOTAUpdater            ( String Hostname = "", String Uri = "", uint16_t Port = 80, byte v1 = 0, byte v2 = 0, byte v3 = 0, byte v4 = 0);
  void                  Begin                   ( String Hostname, String Uri, uint16_t Port = 80, byte v1 = 0, byte v2 = 0, byte v3 = 0, byte v4 = 0);
  Version              &Handle                  ( uint16_t intervalInSeconds =  120); // 2 hours
  void                  SetPinStatus            ( uint8_t FlashPinLed=-1, bool FlashPinNormalStatus= true);
  Error                 Update                  ( void );
  Error                 Update                  ( uint8_t FlashPinLed, bool FlashPinNormalStatus);
  void                  SetCurrentVersion       ( byte v1, byte v2, byte v3, byte v4);
  bool                  ExistNewVersion         ( uint16_t intervalInSeconds = 120); // 2 hours
  Version              &GetNewVersion           ( void );
  Version              &GetCurrentVersion       ( void );
};


#endif

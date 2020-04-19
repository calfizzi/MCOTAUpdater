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

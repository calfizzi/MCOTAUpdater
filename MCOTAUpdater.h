#pragma once
#ifndef MCOTAUpdater_h
#define MCOTAUpdater_h
#include <Arduino.h>


class MCOTAUpdater
{
public:
  typedef enum Error_e
  {
    MCOTA_UPDATE_FAILED,
    MCOTA_UPDATE_NO_UPDATES,
    MCOTA_UPDATE_OK,
    MCOTA_UPDATE_NO_WEB_SITE,
    MCOTA_UPDATE_NO_VALID_URI
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
  Version _NewVersion;
  Version _CurrentVersion;
  String  _Hostname;
  String  _Uri;
private:
  void     _init              ( String Hostname, String Uri);
  String   _FindJsonKey       ( String Data, String SearchFor);
public:
            MCOTAUpdater      ( String Hostname, String Uri, byte v1 = 0, byte v2 = 0, byte v3 = 0, byte v4 = 0);
  Version  &Handle            ( uint16_t intervalInSeconds =  120); // 2 hours
  Error     Update            ( uint8_t PinLed=-1, bool NormalStatus=1);
  void      SetCurrentVersion ( byte v1, byte v2, byte v3, byte v4);
  bool      ExistNewVersion   ( uint16_t intervalInSeconds = 120); // 2 hours
  Version  &GetNewVersion     ( void );
  Version  &GetCurrentVersion ( void );
};


#endif

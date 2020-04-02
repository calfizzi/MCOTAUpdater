# MCOTAUpdater
This Library works on ESP8266 and ESP32 to manage OTA update of your ESP board.

copy in your Website (your path) the SimpleUpdate.bin generated

copy a json (text format) file named your SimpleUpdate.bin.version.json

in that file you have to write your SimpleUpdate.bin version like following row:

{"v1":1,"v2":0,"v3":0,"v4":0}

## Constructor
  `MCOTAUpdater(Your_Host_Name, Your_URI_path, Version_Field_1, Version_Field_2, Version_Field_3,, Version_Field_4)`

## Methods:
```
  Version  &Handle            ( uint16_t intervalInSeconds =  120);      // 2 hours check if new version exist and return it
  
  Error     Update            ( uint8_t PinLed=-1, bool NormalStatus=1); // PIN flash during update
  
  void      SetCurrentVersion ( byte v1, byte v2, byte v3, byte v4);     // to change the current version
  
  bool      ExistNewVersion   ( uint16_t intervalInSeconds = 120);       // 2 hours
  
  Version  &GetNewVersion     ( void );
  
  Version  &GetCurrentVersion ( void );
```
  
  

# MCESPInternetDateTime
This Library works on ESP8266 and ESP32 to get Date Time and Time Zone using your IP.

The library use the site:

  http://worldtimeapi.org/
  
To get Data/Time information using Api:

  http://worldtimeapi.org/api/ip
  
## Methods:
  time_t    **GetTime ( void )**; // Call the Api and get Current Time
  
  void      **ToSerial( void )**; // Print to serial all the information Got.
  
  
## Properties:
  bool                **IsDayLightSaving**; // is true if is in Day Light Saving period
  
  uint32_t            **TimeZoneOffsetInSeconds**; // seconds offset for your zone including Day Light Saving period
  
  time_t              **UTCDateTime**; // UTC Date/Time 
  
  time_t              **DateTime**; // Current Date/Time
  
  String              **Timezone**; // TimeZone description
  
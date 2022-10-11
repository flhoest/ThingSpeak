/*
   ______ __           __          _______         __              
  |   __ \  |--.-----.|  |_.-----.|   |   |.-----.|  |_.-----.----.
  |    __/     |  _  ||   _|  _  ||       ||  -__||   _|  -__|   _|
  |___|  |__|__|_____||____|_____||__|_|__||_____||____|_____|__|  
                                                     v 0.9              
  (c) 2022 - Frederic Lhoest (and Anthony Guerrier)
  
  Send solar power productionto Thingspeak
  ESP8266 + SCT-013 100A + Thingspeak
  Mostly inspired by anthonyguerrier@free.fr from https://www.youtube.com/watch?v=smgw67uRQmw (in French)

  https://diyi0t.com/esp8266-wemos-d1-mini-tutorial/
  https://siytek.com/guide-to-the-wemos-d1-mini/

  WeMos D1 PIN naming convention : 
  static const uint8_t D0   = 16;
  static const uint8_t D1   = 5;
  static const uint8_t D2   = 4;
  static const uint8_t D3   = 0;
  static const uint8_t D4   = 2;  
  static const uint8_t D5   = 14;
  static const uint8_t D6   = 12;
  static const uint8_t D7   = 13;
  static const uint8_t D8   = 15;
  static const uint8_t RX   = 3;
  static const uint8_t TX   = 1;

*/

#include <ESP8266WiFi.h>
#include <EmonLib.h>

// Section related to LED (WIFI + DATA)
int wifiPIN = 16;    // pin D0=16
int dataPIN = 2;     // pin D4=2

// Current detected when no wire is enclosed by the clamp
float ghostCurrent = 0.3;

// Substitute by API Key write of your own thingspeak channel
String apiKey = "<your_write_api_key";


// Substitute by your own Wifi SSID and password
const char* ssid = "<your_ssid>";
const char* password = "<your_password>";
const char* server = "api.thingspeak.com";

// Substitute by you local volatage : 110 for US 220 for rest of the world
const int localVoltage = 220;

// Create a energy monitoring instance
EnergyMonitor emon1;  

WiFiClient client;

void setup() 
{
  double Irms;

  // Prepare LEDs for status display (DATA and WIFI)
  pinMode(wifiPIN, OUTPUT);       
  digitalWrite(wifiPIN, LOW);  

  pinMode(dataPIN, OUTPUT);       
  digitalWrite(dataPIN, LOW);  
  
  Serial.begin(115200);
  delay(10);
  Serial.println("");
  Serial.print("Connecting to  :  ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("");
  // Trying to connect to WiFi
  while (WiFi.status() != WL_CONNECTED) 
  {  // LED flashing fast
    delay(125);
    Serial.print(".");
    digitalWrite(wifiPIN, HIGH);
    delay(125);
    Serial.print(".");
    digitalWrite(wifiPIN, LOW);
  }

  digitalWrite(wifiPIN, HIGH);
  Serial.println("");
  Serial.println("WiFi connceted!");
  Serial.print("IP address: "); Serial.println(WiFi.localIP()); 
  Serial.print("MAC: "); Serial.println(WiFi.macAddress());
  // Display "Received Signal Level" - The lower, the best. -35 is much better than -90
  Serial.print("RSL: "); Serial.println(WiFi.RSSI());

  /* 
     The YHDC SCT-013-000 CT has 2000 turns, so the secondary peak current will be:
     Secondary peak-current = Primary peak-current / no. of turns = 141.4 A / 2000 = 0.0707A
     Ideal burden resistance = (AREF/2) / Secondary peak-current = 2.5 V / 0.0707 A = 35.4 Ohm
  */

  // emon1.current(0, 62);             
  emon1.current(0, 64.15);             // Current: input pin, calibration  => current constant = (100 / 0.050) / 33 = 60.60

/*
  Inverter read : 306 W
  Value measured : 295 W
  
  306 / 295 -> 1.03
  -> 62.282 * 1.03 = 64.15
  
  So far 64.15 is rhe best correction ratio according to my measures
*/

  // Init emon: run the routine 20 times to find the "0" level
  Serial.println(); Serial.print("Emon Initialisation ");

  for (int i = 0; i < 20; i++) {  // LED flasing slow
    digitalWrite(dataPIN, HIGH); // LED on
    Serial.print(".");
    Irms = emon1.calcIrms(1480);  // Calculate Irms only
    delay(250);

    digitalWrite(dataPIN, LOW); // LED off
    Serial.print(".");
    Irms = emon1.calcIrms(1480);  // Calculate Irms only
    delay(250);
  }
  Serial.println(" Init done.");
}

void loop() 
{
    double Irms=0;
    double tmp=0;
    double Debug=0 ;

    Serial.println("");
    Serial.println("Taking 40 samples and compute average");
    
    /* Note : thingspeak requires a minimum of 15 secs between two data updates The logic below is sending
     one measure every 20 secs, so we are good !
     */
     
    for (int i = 0; i < 40; i++) 
    {    
      // Compute Irms mean value over 20 seconds
      tmp=emon1.calcIrms(1480);   
      // Deducting the measured current when no wire enclosed in the clamp - the ghost current
      tmp -= ghostCurrent;
      
      Irms += tmp;
      Debug = tmp;
      if(Debug < 0) Debug=0;
      
      Serial.print("* DEBUG : Sample #");
      Serial.print(i);
      Serial.print(" -> ");
      Serial.print(Debug);
      Serial.print(" A. (");
      Serial.print(Debug * localVoltage);
      Serial.println(" Watt)");
      delay(500);
    }
  
    // Averaging
    Irms /= 40;
    Serial.print("Average : ");
    Serial.print(abs(Irms));
    Serial.println(" A");
    
    // If power value is below 30 W then consider there is no real production -> 0 Watt    
    if ((Irms * localVoltage) < 30) Irms = 0; 

    if (client.connect(server, 80)) 
    {
      String postStr = apiKey;
      postStr += "&field1=";
      postStr += String(Irms);
      postStr += "&field2=";
      postStr += String(Irms * localVoltage);
      postStr += "\r\n\r\n";

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);

      // Blink DATA LED 8 times to confirm data has been sent
      for (int l = 0; l< 8; l++)
      {
        digitalWrite(dataPIN, HIGH);  // LED off
        delay(80);
        digitalWrite(dataPIN, LOW);  // LED on 
        delay(80);
      }
    
      Serial.println("");
      Serial.print("*** Current: ");
      Serial.print(Irms);
      Serial.print(" A. Power: ");
      Serial.print(Irms * localVoltage);
      Serial.println(" W (sent to ThingSpeak).");
  }
  client.stop();

  Serial.println("Next data reading in 20 secs...");

  //  delay(20000);  // delay replaced by Irms mean value computed above
}

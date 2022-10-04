/*
  Send solar power productionto Thingspeak
  ESP8266 + SCT-013 100A + Thingspeak
  Mostly inspired by anthonyguerrier@free.fr from https://www.youtube.com/watch?v=smgw67uRQmw (sorry in French)

  https://diyi0t.com/esp8266-wemos-d1-mini-tutorial/

  PIN Reference for Wemos : 
  static const uint8_t D0   = 16;
  static const uint8_t D1   = 5;
  static const uint8_t D2   = 4;
  static const uint8_t D3   = 0;
  static const uint8_t D4   = 2;     <- never plug anything on this one it will crash it !
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
int wifiPIN = 16;    //D0=16
int dataPIN = 2;     //D4=2

// Current detected when no wire is enclosed by the clamp
float ghostCurrent = 0.3;

// Substitute by API Key write of your own thingspeak channel
String apiKey = "DIP7U1R77ALBARFK";

EnergyMonitor emon1;  // Create an instance

// Substitute by your own Wifi SSID and password
const char* ssid = "<your_ssid>";
const char* password = "<your_password>";

const char* server = "api.thingspeak.com";

WiFiClient client;

void setup() 
{

  // Prepare LEDs for status display (DATA and WIFI)
  pinMode(wifiPIN, OUTPUT);       
  digitalWrite(wifiPIN, LOW);  

  pinMode(dataPIN, OUTPUT);       
  digitalWrite(dataPIN, LOW);  

  double Irms;

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
  Serial.print("RSL: "); Serial.println(WiFi.RSSI());

  /* 
     The YHDC SCT-013-000 CT has 2000 turns, so the secondary peak current will be:
     Secondary peak-current = Primary peak-current / no. of turns = 141.4 A / 2000 = 0.0707A
     Ideal burden resistance = (AREF/2) / Secondary peak-current = 2.5 V / 0.0707 A = 35.4 Î©
     NB : Ce calcul pourrait Ãªtre modifiÃ© lors du montage rÃ©el, 22 ohms pour une carte en 3.3v.

*/
  emon1.current(0, 64.15);             // Current: input pin, calibration  => current constant = (68.6 Ã· 0.050) Ã· 33 = 41.6 @ Aref 3.2 Volts and R = 33
/*

From rectifier : 306 W
Meseared from SCT-013 : 295 W

306/295 -> 1.03
-> 62.282*1.03 = 64.15

So far 64.15 is rhe best correction ratio according to my measures

 */

  // Init emon: run the routine 20 times to find the "0" level
  Serial.println(); Serial.print("Emon Initialisation ");

  for (int i = 0; i < 10; i++) {        // LED flasing slow
    digitalWrite(dataPIN, HIGH);        // LED on
    Serial.print(".");
    Irms = emon1.calcIrms(1480);        // Calculate Irms only
    delay(500);

    digitalWrite(dataPIN, LOW);         // LED off
    Serial.print(".");
    Irms = emon1.calcIrms(1480);        // Calculate Irms only
    delay(500);
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
  
    for (int i = 0; i < 40; i++) 
    {    
      // Compute Irms mean value over 20 seconds
      tmp=emon1.calcIrms(1480);   // Calculate Irms only
      // Deducting the measured current when no wire enclosed in the clamp
      tmp -= ghostCurrent;
      
      Irms += tmp;
      Debug=emon1.calcIrms(1480);
      Serial.print("* DEBUG : Sample #");
      Serial.print(i);
      Serial.print(" -> ");
      Serial.print(Debug);
      Serial.print(" A. (");
      Serial.print(abs(tmp * 220));
      Serial.println(" Watt)");
      delay(500);
    }
  
    // Averaging
    Irms /= 40;
    Serial.print("Average : ");
    Serial.print(Irms);
    Serial.println(" A");
    
    // If power value is below 30 W then consider there is no current -> 0 Watt    
    if ((Irms*220) < 30) Irms = 0; 

    if (client.connect(server, 80)) 
    {
      String postStr = apiKey;
      postStr += "&field1=";
      postStr += String(Irms);
      postStr += "&field2=";
      postStr += String(Irms * 220);
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

      // Blink DATA LED 8 times to confirm data sent
      for (int l = 0; l< 8; l++)
      {
        digitalWrite(dataPIN, HIGH);      // LED off
        delay(80);
        digitalWrite(dataPIN, LOW);       // LED on 
        delay(80);
      }
    
      Serial.println("");
      Serial.print("*** Current: ");
      Serial.print(Irms);
      Serial.print(" A. Power: ");
      Serial.print(Irms * 220);
      Serial.println(" W (sent to ThingSpeak).");

  }
  client.stop();

  Serial.println("Next data reading in 20 secs...");

  // delay repaced by Irms mean value computed above
  //  delay(20000);  
}

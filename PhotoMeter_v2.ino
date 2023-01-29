/*
   ______ __           __          _______         __              
  |   __ \  |--.-----.|  |_.-----.|   |   |.-----.|  |_.-----.----.
  |    __/     |  _  ||   _|  _  ||       ||  -__||   _|  -__|   _|
  |___|  |__|__|_____||____|_____||__|_|__||_____||____|_____|__|  
                                                     v 2.0              
  (c) 2023 - Frederic Lhoest (and Anthony Guerrier)
  Special thanks to Dominique B.
  
  Send solar power productionto Thingspeak
  ESP8266 + SCT-013 100A + Thingspeak
  Mostly inspired by anthonyguerrier@free.fr from https://www.youtube.com/watch?v=smgw67uRQmw (in French)

  https://diyi0t.com/esp8266-wemos-d1-mini-tutorial/
  https://siytek.com/guide-to-the-wemos-d1-mini/

  WeMos D1 PIN naming convention : 
  static const uint8_t D0   = 16;
  static const uint8_t D1   = 5;
  static const uint8_t D2   = 4; <- SDA
  static const uint8_t D3   = 0;
  static const uint8_t D4   = 2;  
  static const uint8_t D5   = 14;
  static const uint8_t D6   = 12;
  static const uint8_t D7   = 13;
  static const uint8_t D8   = 15;
  static const uint8_t RX   = 3;
  static const uint8_t TX   = 1;

*/

// ********************
// Include section
// ********************

#include <ESP8266WiFi.h>
#include <EmonLib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -------------------------------------------
//              Variable section
// -------------------------------------------

#define SCREEN_WIDTH 128                  // OLED display width, in pixels
#define SCREEN_HEIGHT 64                  // OLED display height, in pixels
#define OLED_RESET     -1                 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C               // < See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Section related to LED (WIFI + DATA)
int wifiPIN = 16;    // pin D0=16
int dataPIN = 2;     // pin D4=2

// Counter for screen shutoff
int screenTimer;

// Define number of sec screen must stay on - 2 mins is a good practice (120 secs)
int screenOn = 120;

// Push button PIN definition
int button_pin = 12;     // D6 = 12

// Current detected when no wire is enclosed by the clamp
float ghostCurrent = 0.3;

// Substitute by API Key write of your own thingspeak channel
// Production Channel
String apiKey = "xxxxxxxxxxxxxxx";

// Substitute to the hostname you want to see on your own network (optional)
String newHostname = "PhotoMeter_v2_House";

// Substitute by your own Wifi SSID and password
char* ssid = "<SSID>";
char* password = "<SSID_Password>";
char* server = "api.thingspeak.com";

// Substitute by you local volatage : 110 for US 220 for rest of the world
const int localVoltage = 220;

// Create a energy monitoring instance
EnergyMonitor emon1;  

// Create a wifi client instance
WiFiClient client;

// Start value for average power
double Average=0;

// Actions in the setup section are only executed once at power on
// -------------------------------------------
// WiFi connect
// -------------------------------------------

void wifiConnect(char* net,char* passwd, String host_name)
{
  
  Serial.println("");
  Serial.print("Connecting to  :  ");
  Serial.println(net);

  WiFi.mode(WIFI_STA);

  //Set new hostname
  WiFi.hostname(host_name.c_str());
  
   // Connect to WiFi
  WiFi.begin(net, passwd);

  Serial.println("");
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
  Serial.println("WiFi connected!");
  Serial.print("IP address: "); Serial.println(WiFi.localIP()); 
  Serial.print("MAC: "); Serial.println(WiFi.macAddress());
  
  // Display "Received Signal Level" - The lower, the best. -35 is much better than -90
  Serial.print("RSL: "); Serial.println(WiFi.RSSI());
}

// -------------------------------------------
// Display Initialization on blanck screen
// -------------------------------------------

void showInitScreen()
{
  display.clearDisplay();

  // Initialization...
  display.setTextSize(1);             
  display.setTextColor(SSD1306_WHITE);    
  display.setCursor(1,34);              
  display.println(F(" Initializing..."));
  display.display();  
}
// -------------------------------------------
// Init Energy Monitor
// -------------------------------------------

void emonInit()
{
  /* 
     The YHDC SCT-013-000 CT has 2000 turns, so the secondary peak current will be:
     Secondary peak-current = Primary peak-current / no. of turns = 141.4 A / 2000 = 0.0707A
     Ideal burden resistance = (AREF/2) / Secondary peak-current = 2.5 V / 0.0707 A = 35.4 Ohm
  */

  double Irms;
  
  // emon1.current(0, 62);             
  emon1.current(0, 64.15);             // Current: input pin, calibration  => current constant = (100 / 0.050) / 33 = 60.60

  //emon1.current(0, 108);             // Current: input pin, calibration  => current constant = (100 / 0.050) / 33 = 60.60
/*
    Inverter read : 306 W
    Value measured : 295 W

    306 / 295 -> 1.03
    -> 62.282 * 1.03 = 64.15
  
    So far 64.15 is rhe best correction ratio according to my measures
  */

  // Init emon: run the routine 20 times to find the "0" level
  Serial.println(); Serial.print("Emon Initialisation ");

  for (int i = 0; i < 20; i++) 
  {  // LED flasing slow
    digitalWrite(dataPIN, HIGH); // LED on
    Serial.print(".");
    Irms = emon1.calcIrms(1480);  // Calculate Irms only
    delay(250);

    digitalWrite(dataPIN, LOW); // LED off
    Serial.print(".");
    Irms = emon1.calcIrms(1480);  // Calculate Irms only
    delay(250);
  }
}

// -------------------------------------------
// Send data to ThingSpeak
// -------------------------------------------

void sendData(char* server, int  port, String apiKey, double field1, double field2)
{
  if (client.connect(server, port)) 
   {
        String postStr = apiKey;
        postStr += "&field1=";
        postStr += String(field1);
        postStr += "&field2=";
        postStr += String(field2);
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
        Serial.print(field1);
        Serial.print(" A. Power: ");
        Serial.print(field2);
        Serial.println(" W (sent to ThingSpeak).");
    }
    client.stop();
   }

// -------------------------------------------
// Turn screen off
// -------------------------------------------

void turnScreenOff()
{
  display.clearDisplay();
  display.display();  
}

// -------------------------------------------
// Display top section of the screen
// -------------------------------------------

void showTopSection(double power)
{
  // Remove previous text
  display.setCursor(5,15);                              // Start at top-left corner
  display.setTextColor(SSD1306_WHITE,SSD1306_BLACK);    // Draw white text
  display.print("         ");
  display.display();

  display.setTextSize(1);                               // Normal 1:1 pixel scale
  //display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.setCursor(5,2);                               // Start at top-left corner
  display.println(F("Power     "));

  display.setTextSize(2);                               // Normal 1:1 pixel scale
  display.setCursor(5,15);                              // Start at top-left corner
  display.setTextColor(SSD1306_WHITE);                  // Draw white text
  display.print(power);
  display.print(F(" W"));
  display.display();

}

// -------------------------------------------
// Display botom section of the screen
// -------------------------------------------

void showBottomSection(int sec)
{
  // Remove previous text
  display.setCursor(5,46);                              // Start at top-left corner
  display.setTextColor(SSD1306_WHITE,SSD1306_BLACK);    // Draw white text
  display.print("         ");
  display.display();
  
  display.setTextSize(1);                               // Normal 1:1 pixel scale
  display.setCursor(5,34);                              // Start at top-left corner
  display.println(F("Next measure in"));

  display.setTextSize(2);                               // Normal 1:1 pixel scale
  display.setCursor(5,46);                              // Start at top-left corner
  display.setTextColor(SSD1306_WHITE);                  // Draw white text
  display.print(sec);
  display.print(F(" sec"));
  display.display();
}

// -------------------------------------------
// Display square around data
// -------------------------------------------

void showRectangle()
{
  // Draw square around the screen
  display.drawLine(0, 0, 127, 0, WHITE);
  display.drawLine(127, 0, 127, 63, WHITE);
  display.drawLine(0, 63, 127, 63, WHITE);
  display.drawLine(0, 31, 127, 31, WHITE);
  display.drawLine(0, 0, 0, 63, WHITE);
  display.display();
}

// -------------------------------------------
// Setup section
// -------------------------------------------

void setup() 
{
  // Init serial console
  
  Serial.begin(115200);
  
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("Starting setup section.");

  // Init a variable counting for how long the screen is "ON"
  screenTimer=0;

  // Configure a Pullup resistor on pin D6 (to read button value)
  pinMode(D6, INPUT_PULLUP);
  
  // Initialisation for OLED screen
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);                // Don't proceed, loop forever
  }
  
  // Display Initialization message on the screen
  showInitScreen();

  // Prepare LEDs for status display (DATA and WIFI)
  pinMode(wifiPIN, OUTPUT);       
  digitalWrite(wifiPIN, LOW);  

  pinMode(dataPIN, OUTPUT);       
  digitalWrite(dataPIN, LOW);  
  
  // Connect to the WiFi with the provided settings
  wifiConnect(ssid,password,newHostname);
  
  // Calibrate energy monitor module
  emonInit();

  Serial.println(" Init done.");
}

// ===================================================
// Main loop
// ===================================================

void loop() 
{
    double Irms=0;
    double tmp=0;
    double Debug=0;
    int j;

    Serial.println("");
    Serial.println("=== Starting Main loop===");
    
    Serial.println("");
    Serial.println("Taking 40 samples and compute average");
    
    /* Note : thingspeak requires a minimum of 15 secs between two data updates The logic below is sending
     one measure every 20 secs, so we are good !
     */
     
    if(screenTimer < screenOn)
    {
      showRectangle();
      showTopSection(abs(Average)*localVoltage);
    }

    // maintain secondary index for countdown on screen
    j=20;

    for (int i = 0; i < 40; i++) 
    {    
      // Compute Irms mean value over 20 seconds
      tmp=emon1.calcIrms(1480);  
      
      // Deducting the measured current when no wire enclosed in the clamp - the ghost current
      tmp -= ghostCurrent;
      
      // If measured power - ghostCurrent is below zero, value is 0
      Debug = tmp;
      if(Debug < 0) Debug=0;

      // Adding the measured power to the total of measure in order to compute average
      Irms += Debug;
      
      Serial.print("* DEBUG : Sample #");
      Serial.print(i);
      Serial.print(" -> ");
      Serial.print(Debug);
      Serial.print(" A (");
      Serial.print(Debug * localVoltage);
      Serial.println(" Watt).");

      if(digitalRead(button_pin) == LOW)
      {
        Serial.print("** Button PRESSED -> ");
        Serial.print("Wakeup screen for another ");
        Serial.print(screenOn);
        Serial.println(" seconds.");
        screenTimer=0;

        // Display other section of the screen once
        showRectangle();
        showTopSection(abs(Average)*localVoltage);
        showBottomSection(j);

      }
      else
      {
        //Serial.println("Button NOT PRESSED");
        //Serial.print("Screen timer is : ");
        //Serial.println(screenTimer);
      }

      // If screen should be on, display the counter update
      if((screenTimer < screenOn) and (i % 2 == 0)) 
      {
          showBottomSection(j);
      }

      // Each 2 iteration, update the counter
      if(i % 2 == 0)
      {
        screenTimer++;
        j--;
      }

      if(screenTimer == screenOn)    
      {
        turnScreenOff();
        Serial.println("** Screen burn protection -> turning screen off");
      }

      // Wait 1/2 sec between each iteration
      delay(500);
    }
  
    // Averaging
    Average = Irms / 40;
    Serial.print("Average : ");
    Serial.print(abs(Average));
    Serial.print(" A");
    Serial.print(" (");
    Serial.print(abs(Average)*localVoltage);
    Serial.println(" W)");
    
    // If power value is below 30 W then consider there is no real production -> 0 Watt    
    if ((Average * localVoltage) < 30) Average = 0; 

    // Send Data to ThingSpeak
    sendData(server,80,apiKey,abs(Average),abs(Average*localVoltage)); 
   Serial.println("Next data computed in 20 secs...");
}

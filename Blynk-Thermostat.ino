#define NAMEandVERSION "Blynk-ThermostatV3.2"
/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************

  This Arduino project is made using Bynk platform and applies as an IoT Thermostat 
  with scheduling functions using the Blynk App for the User Interface.

  WARNING :
  For this example you'll need the following libraries:
  - Blynk
  - DHTesp - https://desire.giesecke.tk/index.php/2018/01/30/esp32-dht11/
  - Arduino OTA - https://github.com/esp8266/Arduino/blob/master/libraries/ArduinoOTA
  - OLED SSD1306 - 

  
 Virtual Pins: 

 V10 - Actual Temp 
 V11 - Actual Humidity 
 V12 - Time Interval Input 
 V13 - Temp set value 
 V14 - LED status for heating/no heating status
 V15 - TempSet Increment Ajustment by app 
 V16 - Switch between Scheduled Interval or Manual
 V17 - GPSTrigger - GPS trigger for the selected radius
 V18 - GPSAutoOff - Switch for Activating the GPSAutoOff
 V19 - LedTimerInterval for monitoring the interval status  
 V20 - ledGPSTrigger for monitoring the proximity to the house
 V21 - ledGPSTrigger In radius monitoring


 *************************************************************/

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

//#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
//#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define DHTPIN 10          // Temp sensor hardware pin on SD3/GPIO10
#include "DHTesp.h"
DHTesp dht;
#include <TimeLib.h>
#include <WidgetRTC.h>

WidgetRTC rtc;
BlynkTimer timer;

#include "SSD1306.h" // alias for #include "SSD1306Wire.h"
 SSD1306  display(0x3c, D7, D6); // SDA, SCL , GPIO 13 and GPIO 12


WidgetLED ledHeatingStatus(V14);
WidgetLED ledInterval(V19);
WidgetLED ledGPSAutoOff(V20);
WidgetLED ledGPSTrigger(V21);

String display_temp;
String display_humid;
String dispTempSet;

int tempset; // tempset value from pin V13
int tempset2; // lower treshold for triggering the heating
int connectionattempts;  // restart the mcu after to many connection attempts


float h;  //temperature value
float t ; //humidity value
String Interval;

int StartHour = 0;
int StopHour = 0;
int StartMinute = 0;
int StopMinute = 0;
int Hour = 0;
int Minute = 0;


bool HEATING;
bool STOPPED;
bool interval;

bool scheduled;
bool GPSTrigger;
bool GPSAutoOff;

bool result;
bool connection;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "auth";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Home";
char pass[] = "pass";

#define relay D1 //  relay  on GPIO5 , D1

#define upPin D0 // increment tempset - GPIO10
#define downPin D5 // decrement tempset - GPIO14



bool UpState;
bool prevUpState;
bool DownState;
bool prevDownState;



//Using Static IP
byte arduino_mac[] = { 0x2C, 0x3A, 0xE8, 0x0E, 0x5A, 0x36 };
IPAddress arduino_ip ( 192,  168,   1,  10);
IPAddress dns_ip     ( 192,  168,   1,   1);
IPAddress gateway_ip ( 192,  168,   1,   1);
IPAddress subnet_mask(255, 255, 255,   0);



void setup()
{
  WiFi.hostname(NAMEandVERSION);
  WiFi.mode(WIFI_STA);
  // Debug console
  Serial.begin(9600);
  
  pinMode (relay, OUTPUT);
  pinMode (upPin, INPUT);
  pinMode (downPin, INPUT);
  pinMode (DHTPIN, INPUT);
  
  WiFi.config(arduino_ip, gateway_ip, subnet_mask);
  
 // OTAdebug();
  displayInitSeq();
  
  // Setup a function to be called every second
  timer.setInterval(8000L, sendTemp);
  timer.setInterval(5000L, HeatingLogic);
  timer.setInterval(3000L, displayData); 
  timer.setInterval(5000L, timesync);   
  timer.setInterval(10000L, connectionstatus);
}

void loop()
{
  Blynk.run();
  timer.run();
  //ArduinoOTA.handle();
  dispTempSet = tempset;
  ButtonsUpDown();
}

void displayInitSeq()
{
  dht.setup(DHTPIN, DHTesp::DHT22); // Connect DHT sensor to GPIO 10
  delay(dht.getMinimumSamplingPeriod());
  h = dht.getHumidity();
  t = dht.getTemperature();
  if (isnan(h) || isnan(t)) {
   Serial.println("Failed to read from DHT sensor!");
    return;
  }

  yield();
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 0, String(NAMEandVERSION)); 
  display.display();
  WiFi.begin(ssid, pass);
  delay(5000);  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    display.drawString(64, 10, "Connection to Wifi...");
    display.drawString(64, 20, "FAILED!");
    delay(2000);
    display.display();
    ESP.restart(); 
  }
  Serial.println("");
  Serial.println("WiFi connected");
  display.drawString(64, 20, "WiFi connected!");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); 
  display.drawString(64, 30, "Local IP: " + String(WiFi.localIP().toString()) );
  display.display();  
  while (Blynk.connected() == false) {
    display.drawString(64, 40, "Connecting to Server...");
    delay(1000);
    display.display();
    Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,3), 8441);
  }
  display.drawString(64, 50, "Connected to Server!");
  Serial.println("Connected to Blynk server");
  display.display();
  yield();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
}

//void OTAdebug()
//{
//  // Port defaults to 8266
//   ArduinoOTA.setPort(8266);
//
//  // Hostname defaults to esp8266-[ChipID]
//  // ArduinoOTA.setHostname("myesp8266");
//
//  // No authentication by default
//  // ArduinoOTA.setPassword("admin");
//
//  // Password can be set with it's md5 value as well
//  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
//  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
//
//  ArduinoOTA.onStart([]() 
//  {
//    yield();
//    String type;
//    if (ArduinoOTA.getCommand() == U_FLASH)
//      type = "sketch";
//    else // U_SPIFFS
//      type = "filesystem";
//  
//    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
//     Serial.println("Start updating " + type);
//  });
//  
//  ArduinoOTA.onEnd([]() 
//  {
//   Serial.println("\nEnd");
//  });
//  
//  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
//  {
//   Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
//  });
//  ArduinoOTA.onError([](ota_error_t error) {
//   Serial.printf("Error[%u]: ", error);
//    if (error == OTA_AUTH_ERROR)Serial.println("Auth Failed");
//    else if (error == OTA_BEGIN_ERROR)Serial.println("Begin Failed");
//    else if (error == OTA_CONNECT_ERROR)Serial.println("Connect Failed");
//    else if (error == OTA_RECEIVE_ERROR)Serial.println("Receive Failed");
//    else if (error == OTA_END_ERROR)Serial.println("End Failed");
//    yield();
//  });
//  ArduinoOTA.setHostname(NAMEandVERSION); // OPTIONAL NAME FOR OTA
//  yield();
//  ArduinoOTA.begin();
//  yield();  
//}

BLYNK_CONNECTED(){
  Blynk.syncVirtual(V10, V11, V12, V13,V14, V16,V17,V18,V19,V20,V21);  // synk on connection
yield();
}
BLYNK_WRITE(V12) {  
    TimeInputParam t(param);
    StartHour = t.getStartHour();
    StopHour = t.getStopHour();
    StartMinute = t.getStartMinute();
    StopMinute = t.getStopMinute();
    Hour = hour();
    Minute = minute();
    yield(); 
}


BLYNK_WRITE(V13)
{
  //restoring int value
  tempset = param.asInt();
  Serial.println("tempset updated");
  yield();
}

BLYNK_WRITE(V15) // ajust tempset by app buttons
{
  int stepperValue = param.asInt();
  if (stepperValue == 1)       // assigning incoming value from pin V1 to a variable
  {
    tempset++;
    Blynk.virtualWrite(V13, tempset);
    Serial.println(tempset);
    displayData();
    yield();
  }
  else if (stepperValue == -1) 
  {
    tempset--;
    Blynk.virtualWrite(V13, tempset);
    Serial.println(tempset);
    yield();
    displayData();
  }
  // process received value
  //Blynk.virtualWrite(V13, tempset);
  yield();
  //Serial.println(param.asInt());  
  Serial.println(stepperValue);  
}

BLYNK_WRITE(V16)   // ON 1 = scheduled   - OFF 0 = manual(check temperature allways)
{ 
  //restoring int value
  scheduled = param.asInt();
  Serial.println();
  Serial.print("scheduled=");
  Serial.println(scheduled);
  Serial.println();
  yield();
  //Blynk.virtualWrite(V16, scheduled); //update state of the buttos as it is on the mcu
}

BLYNK_WRITE(V17)   // ON 1 = you are in the radius, youre at home  - OFF 0 = you left the home
{ 
  //restoring int value
  GPSTrigger = param.asInt();
  //Store the value on server?
  Serial.println();
  Serial.print("GPSTrigger=");
  Serial.println(GPSTrigger);
  Serial.println();
  yield();
  Blynk.virtualWrite(V17, GPSTrigger); //update state of the buttos as it is on the mcu
  if(scheduled == 0)
  {
    if (GPSTrigger == 1)
    {
      Blynk.notify("C YA!");
    }
    else
    {
      Blynk.notify("Welcome Back!");
    }
  }
}

BLYNK_WRITE(V18)   // ON 1 = GPSAutoOFF is activated, turn off the heating if it is on manual and there is nobody home   - OFF 0 = deactivated
{ 
  //restoring int value
  GPSAutoOff = param.asInt();
  Serial.println();
  Serial.print("GPSAutoOff=");
  Serial.println(GPSAutoOff);
  Serial.println();
  yield();
}

void timesync() //run this every 5 seconds
{
    //Every time the hours interval is received from server/app it runs the whole time check.To be improved!!!!!
    //had a crush right here last time
    rtc.begin();
    yield();
}


void connectionstatus()
{ 
  connection = Blynk.connected();
  if (connection == 0)
  {
    //Check if it's still connected to wifi!
    if ( WiFi.status() != WL_CONNECTED )
    {
      Serial.println();
      Serial.print("connectionattempts");
      Serial.print(connectionattempts);
      Serial.println();
      display.init();
      display.clear();
      display.flipScreenVertically();
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_10);
      Serial.println("Wifi Connection Lost!");
      display.drawString(64,0, " WiFi Connection Lost!");
      display.drawString(64,10, "  RECONNECTING ...");  
      display.drawString(64,20, String("Attempt: ") + String(connectionattempts)); 
      display.display();
      yield();         
    }
    else
    {
        display.drawString(64,0, " Server Connection Lost!"); 
        display.drawString(64,10, "  RECONNECTING ...");  
        display.drawString(64,20, "Attempt: " + String(connectionattempts)); 
        display.display();
        yield();        
    }
    connectionattempts ++;    
  }
  else 
  {
    connectionattempts = 0;
  }
  
  if (connectionattempts > 5)
  {
    Serial.println("Self Restart!");
    delay(3000);
    ESP.restart();  
  }
}


void checkInterval()
{
    String currentTime = String(Hour) + ":" + String(Minute) + ":" + String(second());
    //String currentDate = String(day()) + " " + month() + " " + year();
    Interval = String( String(StartHour) + ":" + String(StartMinute) + " - " + String(StopHour) + ":" + String(StopMinute));
    yield();
    Serial.print("Current time: " + String(currentTime));
    Serial.println();
    Blynk.syncVirtual(V12,V17); //synk the time interval and the gps trigger.
  
  /*

  *23:10 - 23:40
  *2:00 - 2:40
  *2:50 - 2:20
  
  22:10 - 2:00
  10:10 - 2:00
  3:00 - 1:00 
  
  *10:10 - 12:10
  *10:10 - 22:10
    
  */

  // 23:10 - 23:40 && 2:00 - 2:40
  if (Hour == StartHour && StartHour == StopHour && StartMinute < StopMinute) // Less than 1H
  {
    if (Minute >= StartMinute && Minute < StopMinute)
    {
      interval = 1;
      Serial.println("100");
    }
    else
    {
      interval = 0;
      Serial.println("101");
    }
  }
  
  
  // 12:50 - 12:10  12:11
  if (Hour == StartHour && StartHour == StopHour && StartMinute > StopMinute) // 24H - some Minutes
  {
    if (Hour != StartHour)
    {
      interval = 1;
      Serial.println("102");
    }
    else if (Hour == StartHour)
    {
      if (Minute < StartMinute && Minute > StopMinute) 
      {
        interval = 1;
        Serial.println("103");
      }
      else 
      {
        interval = 0;
        Serial.println("104");
      }
    }
  }
  
  // 10:10 - 12:10 && 10:10 - 22:10 - sameday
  if (StartHour < StopHour )
  {
    if (Hour == StartHour)
    {
      if (Minute < StartMinute)
      {
        interval = 0;
        Serial.println("105");
      }
      else if (Minute >= StartMinute)
      {
        interval = 1;
        Serial.println("106");
      }
    }
    
    if (Hour == StopHour)
    {
      if (Minute > StopMinute)
      {
        interval = 0;
        Serial.println("107");
      }
      else if (Minute <= StopMinute)
      {
        interval = 1;
        Serial.println("108");
      }
    }
    
    if (Hour > StartHour && Hour < StopHour)
    {
      interval = 1;
      Serial.println("109");
    }
  }
  
  
  // 22:10 - 2:00 && 10:10 - 2:00 && 3:00 - 1:00  - next day
  if (StartHour > StopHour)
  {
    if (Hour == StartHour)
    {
      if (Minute < StartMinute)
      {
        interval = 0;
        Serial.println("110");
      }
      else if (Minute >= StartMinute)
      {
        interval = 1;
        Serial.println("111");
      }
    }
    
    if (Hour == StopHour)
    {
      if (Minute > StopMinute)
      {
        interval = 0;
        Serial.println("112");
      }
      else if (Minute <= StopMinute)
      {
        interval = 1;
        Serial.println("113");
      }
    }   
    
    //23:10 22:10 - 2:00 && 10:10 - 2:00 && 3:00 - 1:00  - next day
    if (Hour > StartHour)
    {
      interval = 1;
      Serial.println("114");
    }
    // 1:00 // 22:10 - 2:00 && 10:10 - 2:00 && 3:00 - 1:00  - next day
    else if (Hour < StartHour)
    {
      if (Hour > StopHour)
      {
        interval = 0;
        Serial.println("115");
      }
      else if (Hour < StopHour)
      {
        interval = 1;
        Serial.println("116");
      }
    }
  }
  yield();
}



void displayData() {
    dispTempSet = tempset;  
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    connection = Blynk.connected();
    if (connection == 0){
    display.drawString(64, 0, "Disconnected . . .");        
    }
    
     else {
      if (scheduled == 1)
      {
        if (interval == 1)
        {
          display.drawString(0, 0, "Set: " + dispTempSet + "°C    " + Interval);  
        }
//        else 
//        {
//          display.drawString(0, 0, "Set:" + dispTempSet + Interval);            
//        }
      yield();
      }
      else 
      {
        display.drawString(0, 0, "Set:" + dispTempSet + "°C    *   24/7");   
        yield();
      }

    }
    display.setFont(ArialMT_Plain_24);
    display.setTextAlignment(TEXT_ALIGN_CENTER);    
    display.drawString(66, 13, display_temp + "°C");
    display.drawString(66, 40, display_humid + "%");
    display.display();
    yield();
}

void displayDataBig() {
    dispTempSet = tempset;
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(66, 13, "Set:" + dispTempSet + "°C" );   
    display.display();
    yield();

}

void sendTemp()
{
  yield();
  //delay(dht.getMinimumSamplingPeriod());

  h = dht.getHumidity();
  t = dht.getTemperature();

  display_temp = t;
  display_humid = h;
  
 Serial.println(display_temp);
 Serial.println(display_humid);
 
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V10, t); //temperature on virtual pin V5
  Blynk.virtualWrite(V11, h); //humidity on virtual pin V6
  yield();
}


void HeatingLogic()
{
  checkInterval();
  if (scheduled == 1) {
    Serial.println("By Time");
    Serial.println();
    Serial.print("interval=");
    Serial.println(interval);
    Serial.println();
    if (interval == 1 ) {
      TempCompare();  // do the relay thing if the right time comes and the temperature thing
      Serial.println("Heating was turned ON at the right time in interval");
      ledInterval.on();  
      yield();     
    } 
    else {
      HeatOff();
      STOPPED = 0; 
      HEATING = 0;
      ledInterval.off();
      Serial.println("Heating was turned off because the interval is passed");
      yield();
    }
  }
  else { // Not Scheduled!
    if (GPSAutoOff == 0){
      ledGPSAutoOff.off();
      TempCompare();  
      Serial.println("Thermostat is running non stop");   
      yield();
    }
    else 
    { // GPSAutoOff is activated.When its on manual,keep heating only if you are at home
      ledGPSAutoOff.on();
      if (GPSTrigger == 0) //triggering when you leave the home
      {
        ledGPSTrigger.on();
        TempCompare();  
        Serial.println("Thermostat is running again because you are at home!");   
        yield();
      }
      else
      {
        ledGPSTrigger.off();
        HeatOff();
        STOPPED = 0; 
        HEATING = 0;
        Serial.println("Heating was turned off because it's set on manual, but there is nobody at home");
        yield(); 
      }
    }
  }
}

void TempCompare()
{
  tempset2 = tempset-1;   
  // incepe din starea
  // HEATING = 0
  // STOPPED = 0
            
  if (HEATING == 0 && STOPPED == 0){
    if (tempset2 > t){
      HeatOn();
      //Blynk.setProperty(V5, "color", "purple");    
      Serial.println("Heating Just Started!");        
    }
    else {
      HeatOff();
      STOPPED = 0;
      Serial.println("Keep it OFF!");
    }
  }
          
  else if (HEATING == 1){
    if (t > tempset){ // 
      HeatOff();
      STOPPED = 1;
      //Blynk.setProperty(V5, "color", "green");
      Serial.println("Temp was reached.Wait till it drops by 1C");
    }
    else {
      HeatOn();
      STOPPED = 0; 
      //Blynk.setProperty(V5, "color", "red");  
      Serial.println("Heating...");         
    }
  }    
  else if (STOPPED == 1){         
    if (t < tempset2){ // start heating again only if the temp dropped by one deg
      HeatOn();
      //Blynk.setProperty(V5, "color", "red");                             
      STOPPED = 0;
      Serial.println("Temp dropped by one deg.Start heating again");
    }
    else {
      HeatOff();
      Serial.println("Keep it off till it drops by one deg");              
    }
  }
}


void HeatOn() 
{
  digitalWrite(relay, HIGH);
  ledHeatingStatus.on();
  Serial.println("heatOn"); 
  HEATING = 1;
  yield();
}

void HeatOff() 
{
  digitalWrite(relay, LOW);
  ledHeatingStatus.off();
  Serial.println("heatOff"); 
  HEATING = 0;
  yield();
}

void ButtonsUpDown()
{
  UpState = digitalRead(upPin);
  if (UpState != prevUpState) {
    if (UpState == LOW) {
      tempset++;
      Blynk.virtualWrite(V13, tempset);    
      displayDataBig();
     Serial.println("Increased!");
     Serial.println(tempset);
      delay(100);  
      yield();
    }
    prevUpState = UpState;
  }

  DownState = digitalRead(downPin);
  if (DownState != prevDownState) {
    if (DownState == LOW) {
      tempset--;
      Blynk.virtualWrite(V13, tempset);         
      displayDataBig();
     Serial.println("Decreased!");  
     Serial.println(tempset);
      delay(100);            
    }
    prevDownState = DownState;
  }
}

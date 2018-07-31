<<<<<<< HEAD

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
  For this example you'll need Adafruit DHT sensor libraries:
    https://github.com/adafruit/Adafruit_Sensor
    https://github.com/adafruit/DHT-sensor-library

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
 V21 - In radius monitoring


 *************************************************************/

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
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
char auth[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";  

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "SSID";
char pass[] = "WIFIPASS";

#define DHTPIN 10          // Temp sensor hardware pin on SD3/GPIO10 - To check with a different library/different pin
#define relay D1 //  relay  on GPIO5 , D1

#define upPin D0 // increment tempset - GPIO10
#define downPin D5 // decrement tempset - GPIO14

bool UpState;
bool prevUpState;
bool DownState;
bool prevDownState;

#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321

DHT dht(DHTPIN, DHTTYPE, 22);

//Using Static IP
byte arduino_mac[] = { 0x2C, 0x3A, 0xE8, 0x0E, 0x5A, 0x36 };
IPAddress arduino_ip ( 192,  168,   1,  10);
//IPAddress dns_ip     (  8,   8,   8,   8);
IPAddress gateway_ip ( 192,  168,   1,   1);
IPAddress subnet_mask(255, 255, 255,   0);



void setup()
{
  WiFi.hostname("SmartThermostat-V2.0");
  WiFi.mode(WIFI_STA);
  // Debug console
  Serial.begin(9600);
  WiFi.config(arduino_ip, gateway_ip, subnet_mask);
  
  yield();
  
  dht.begin();
  h = dht.readHumidity();
  t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
  
  yield();
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 0, " CONNECTING ...");   
  display.display();
  yield();
//  Blynk.begin(auth, ssid, pass);
  yield();
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 8442);
  //Or for local server:
  Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,3), 8080);


  // Setup a function to be called every second
  timer.setInterval(5000L, sendTemp);
  yield();
  //timer.setInterval(5000L, TempCompare);
  timer.setInterval(10000L, displayData); 
  yield();
  timer.setInterval(2000L, timesync); 
  yield();  
  timer.setInterval(10000L, connectionstatus);
  yield();  
  
  pinMode (relay, OUTPUT);
  pinMode (upPin, INPUT);
  pinMode (downPin, INPUT);
  pinMode (DHTPIN, INPUT);


  
  // Port defaults to 8266
   ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() 
  {
    yield();
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
  
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() 
  {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
  {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    yield();
  });
  ArduinoOTA.setHostname("Smart Thermostat 2.0"); // OPTIONAL NAME FOR OTA
  yield();
  ArduinoOTA.begin();
  yield();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void loop()
{
  yield();
  ArduinoOTA.handle();
  yield();
yield();
  Blynk.run();
yield();
  dispTempSet = tempset;
yield();
  timer.run();
yield();
  ButtonsUpDown();
yield();

yield();  
  if (interval == 1)
  {
    ledInterval.on();
    yield();
  }
  else
  {
    ledInterval.off();
  yield();  
  }

}



BLYNK_CONNECTED(){
  Blynk.syncVirtual(V10, V11, V12, V13,V14, V16,V17,V18,V19,V20,V21);  // synk on connection
yield();
}

void timesync()
{
    Blynk.syncVirtual(V12);
    rtc.begin();
    yield();
}


void connectionstatus()
{
//  result = Blynk.connected();
//  result = Blynk.connect(60);
  connection = Blynk.connected();
  if (connection == 0)
  {
      connectionattempts ++;
      Serial.println();
      Serial.print("connectionattempts");
      Serial.print(connectionattempts);
      Serial.println();
      yield();
      display.init();
      display.clear();
      display.flipScreenVertically();
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_10);
      display.drawString(64, 0, " CONNECTING ...");   
      display.display();
      yield();
  }
  else 
  {
    connectionattempts = 0;
  }
  
  if (connectionattempts == 5)
  {
      ESP.restart();  
  }
}

BLYNK_WRITE(V12) {
  TimeInputParam t(param);
 
  String currentTime = String(hour()) + ":" + minute() + ":" + second();
  String currentDate = String(day()) + " " + month() + " " + year();
  yield();
  Serial.print("Current time: ");
  Serial.print(currentTime);
  Serial.print("   ");
//  Serial.print(currentDate);
//  Serial.println();
  
    Serial.print(String(" Start: ") +
                   t.getStartHour() + ":" +
                   t.getStartMinute() + ":" +
                   t.getStartSecond());
    Serial.print(String("  Stop:") + 
                   t.getStopHour() + ":" +
                   t.getStopMinute() + ":" +
                   t.getStopSecond());
    if(interval== 1)
    {
      Serial.print(" = > Started!");               
    }
    else
    {
      Serial.print(" = > Stopped");
    }
                          
    
    Serial.println();  Serial.println();
    

    //--> Starting hour is the same as the ending hour. 
    if (t.getStartHour() == hour() && hour() == t.getStopHour())   // hours match.check the minutes
    {
      Serial.println("The Hour is Matching.Check the minutes");
      if (t.getStartMinute() <= minute() && t.getStopMinute() > minute() )
      {
        Serial.println("The start/end Hour is the same, start<Minutes<end ");
        interval = 1;
      }
      if (t.getStartMinute() > minute() )
      {
        Serial.println("Minutes are not matching for the interval, Yet.");
        interval = 0;        
      }
      if (t.getStopMinute() < minute() )
      {
        Serial.println("minutes>Stop Interval");
        interval = 0 ;
      }
    }
    // <--- Starting hour is the same as the ending hour. 
    yield();

    // Stop time matches the current hour. Check the minutes.
    if (t.getStopHour() == hour() ) 
    {
      if ( t.getStopMinute() <= minute()  ) 
      {
        Serial.println("StopTime is passed");
        interval = 0;
      }
      if (t.getStopMinute() > minute() && minute() > t.getStartMinute() )
      {
        Serial.println("During the interval. StartMinute<Minute<StopMinute");
        interval= 1;
      }
    }
    if (t.getStopHour() == t.getStartHour() && t.getStopHour() != hour() )
    {
      Serial.println("The Interval is <1H but the current Hour does not match the interval");
      interval = 0;
    }


    // ---> StartHour > StopHour - It ends next day Eg: 15:00 - 14:00 , 06:00 - 05:00 , 06:00 - 01:00
    if (t.getStartHour() > t.getStopHour() ) 
    {
      Serial.println("StartHour > StopHour - It ends next day ");
      if (t.getStartHour() == hour())
      {
        Serial.println("StartHour=Hour - During the interval.Check the minutes");
        if (t.getStartMinute() <= minute() && minute() <= t.getStopMinute() )
        {
          Serial.println("StartHour=Hour StartMinute<=Minute<=EndMinute");
          interval = 1;
        }
        if (t.getStartMinute() > minute() || minute() > t.getStopMinute() )
        {
          Serial.println("StartHour=Hour StartMinute>Minute or Minute>EndMinute");
          interval = 0;
        }
      }
      // eg: interval: 15:00 - 3:00 
      if (t.getStartHour() > hour() && hour() > t.getStopHour() )
      {
        Serial.println("current H>StartH && CurrentH > StopH");
        interval = 0;
      }
      if (t.getStartHour() > hour() && hour() == t.getStopHour() )
      {
        Serial.println("StartH>Hour && Hour==StopH , check minutes");
        if (t.getStopMinute() <= minute() )
        {
          Serial.println("Few minutes over the given interval. Stop!");
          interval = 0;
        }
        else 
        {
          Serial.println("Last Hour.Check the minutes");
          interval = 1;
        }
      }
      yield();
      if(t.getStartHour() > hour() && hour() < t.getStopHour() )
      {
        Serial.println("StartH>H, H<StopH - During the given interval");
        interval = 1;
      }

      //during the interval StartH<H , H>StopH
      if (t.getStartHour() < hour() && hour() > t.getStopHour() )
      {
        Serial.println("during the interval StartH<H , H>StopH");
        interval = 1;
      }

    }
    // ---> StartHour < StopHour
    
    //daca ora de inceput este mai mica decat ora de sfarsit atunci intervalul se incheie in aceeasi zi
    else if (t.getStartHour() < t.getStopHour() ) 
    {
      Serial.println("Ora de sfarsit se incheie in aceeasi zi");
      if (t.getStartHour() < hour() && t.getStopHour() > hour() )  // ora este mai mare decat ora starttime dar mai mica decat stoptime.
      {
        Serial.println("Se afla in intervalul de ore, nu in aceeasi ora ed sfarsit, deci nu e nevoie sa verific minutele");
        interval= 1;
      }

      if (t.getStartHour() == hour() )
      {
        Serial.println("Valoarea orei se potriveste cu intervalul, verifica si minutele");
        if (t.getStartMinute() <= minute())
        {
          Serial.println("Se potrivesc si minutele. in interval");
          interval=1;  
        }
        else 
        {
          Serial.println("Minutele nu se potrivesc, In afara intervalului@");
          interval = 0;
        }
      }
      if (t.getStartHour() > hour() )
      {
        Serial.println("Inca nu s-a apropiat intervalul.In afara intervalului");
        interval = 0;
      }
      if (t.getStopHour() == hour())
      {
        Serial.println("Ultima ora din interval.verifica minutele");
        if (t.getStopMinute() <= minute() )
        {
          Serial.println("A depasit minutele.In afara intervalului!");
          interval = 0;
        }
        else if (t.getStopMinute() > minute() )
        {
          Serial.println("Nu au depasit minutele.In interval");
          interval = 1;
        }
      }
      
    }


yield();
  Serial.println();
}



void displayData() {
    dispTempSet = tempset;  
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    if (connection == 0){
      display.drawString(64, 0, "Disconnected . . .");        
    }
     else {
      if (scheduled == 1)
      {
        if (interval == 1)
        {
          display.drawString(0, 0, "Set:" + dispTempSet + "°C Scheduled-isTimeTo...");  
        }
        else 
        {
          display.drawString(0, 0, "Set:" + dispTempSet + "°C Scheduled-notYet...");            
        }
      yield();
      }
      else 
      {
        display.drawString(0, 0, "Set:" + dispTempSet + "°C * AllwaysOn");   
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
  Blynk.syncVirtual(V12, V16,V17,V18,V14,V19,V20,V21);
  
  h = dht.readHumidity();
  t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
    yield();

  display_temp = t;
  display_humid = h;
  Serial.println(display_temp);
  Serial.println(display_humid);
  
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V10, t); //temperature on virtual pin V5
  Blynk.virtualWrite(V11, h); //humidity on virtual pin V6
yield();
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
  else { // scheduled == 0
    if (GPSAutoOff == 0){
      TempCompare();  
      Serial.println("Thermostat is running non stop");   
      yield();
    }
    if (GPSAutoOff == 1){ // GPSAutoOff is activated.When its on manual,keep heating only if you are at home
      if (GPSTrigger == 0) //triggering when you leave the home
      {
        TempCompare();  
        Serial.println("Thermostat is running again because you came back");   
        yield();
      }
      else if (GPSTrigger == 1)
      {
        HeatOff();
        STOPPED = 0; 
        HEATING = 0;
        Serial.println("Heating was turned off because it's set on manual, but there is nobody at home");
        yield(); 
      }
    }

  }
  
 Blynk.virtualWrite(V13, tempset);       // update tempset at the same time with the temperature   
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
  Blynk.virtualWrite(V13, tempset);
  yield();
  Serial.println(param.asInt());  
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
  if (GPSTrigger == 0)
  {
    ledGPSTrigger.on();
  }
  else if (GPSTrigger == 1) // nobody is at home
  {
    ledGPSTrigger.off();    
  }
  Serial.println();
  Serial.print("GPSTrigger=");
  Serial.println(GPSTrigger);
  Serial.println();
  yield();
  //Blynk.virtualWrite(V17, GPSTrigger); //update state of the buttos as it is on the mcu
}

BLYNK_WRITE(V18)   // ON 1 = GPSAutoOFF is activated, turn off the heating if it is on manual and there is nobody home   - OFF 0 = deactivated
{ 

  //restoring int value
  GPSAutoOff = param.asInt();
  if (GPSAutoOff == 1)
  {
    ledGPSAutoOff.on();
  }
  else if (GPSAutoOff == 0)
  {
    ledGPSAutoOff.off();
  }
  
  Serial.println();
  Serial.print("GPSAutoOff=");
  Serial.println(GPSAutoOff);
  Serial.println();
  yield();
  //Blynk.virtualWrite(V18, GPSAutoOff); //update state of the buttos as it is on the mcu
}


void ButtonsUpDown()
{
   /* Tratează cazul când s-a apăsat up pentru incrementarea tempset. */
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

  /* Tratează cazul când s-a apăsat down pentru decrementarea tempset. */
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

void TempCompare()
{
  yield();
   tempset2 = tempset-1;   
          // incepe din starea
          // HEATING = 0
          // STOPPED = 0
            
          if (HEATING == 0 && STOPPED == 0){
            if (tempset > t){
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
            if (t > tempset){ // prima oprire a termoreleului
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
            if (t < tempset2){ // daca temperatura actuala A SCAZUT cu cel putin un grad fata de cea pe care a avut-o cand s-a oprit abi atunci incepe sa incalzeasca
              HeatOn();
              //Blynk.setProperty(V5, "color", "red");                             
              STOPPED = 0;
              Serial.println("Temp dropped by one deg.Start heating again");
            }
            else {
              HeatOff();
              Serial.println("Keep it off til it drops by one degree");              
            }
          }
}


=======

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
  For this example you'll need Adafruit DHT sensor libraries:
    https://github.com/adafruit/Adafruit_Sensor
    https://github.com/adafruit/DHT-sensor-library

 Virtual Pins: 

 V10 - Actual Temp 
 V11 - Actual Humidity 
 V12 - Time Interval Input 
 V13 - Temp set value 
 V14 - LED status for heating/no heating status
 V15 - TempSet Increment Ajustment by app 
 V16 - Switch between Scheduled Interval or Manual
 V17 - 
 V18 - 
 V19 - LedTimerInterval for monitoring the interval status  
 V20 - 


 *************************************************************/

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

WidgetRTC rtc;
BlynkTimer timer;

#include "SSD1306.h" // alias for #include "SSD1306Wire.h"
 SSD1306  display(0x3c, D7, D6); // SDA, SCL , GPIO 13 and GPIO 12


WidgetLED ledHeatingStatus(V14);
WidgetLED ledInterval(V19);


String display_temp;
String display_humid;
String dispTempSet;

int tempset; // tempset value from pin V13
int tempset2; // lower treshold for triggering the heating
int connectionattempts;  // restard the mcu after to many connection attempts


float h;  //temperature value
float t ; //humidity value


bool HEATING;
bool STOPPED;
bool interval;

bool scheduled;

bool result;
bool connection;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "SSID";
char pass[] = "wifipass";

#define DHTPIN 10          // Temp sensor hardware pin on SD3/GPIO10
#define relay D1 //  relay  on GPIO5 , D1

#define upPin D0 // increment tempset - GPIO10
#define downPin D5 // decrement tempset - GPIO14

bool UpState;
bool prevUpState;
bool DownState;
bool prevDownState;

#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321

DHT dht(DHTPIN, DHTTYPE, 22);
// This function sends Arduino's up time every second to Virtual Pin (5).
// In the app, Widget's reading frequency should be set to PUSH. This means
// that you define how often to send data to Blynk App.




//Using Static IP
byte arduino_mac[] = { 0x2C, 0x3A, 0xE8, 0x0E, 0x5A, 0x36 };
IPAddress arduino_ip ( 192,  168,   1,  10);
//IPAddress dns_ip     (  8,   8,   8,   8);
IPAddress gateway_ip ( 192,  168,   1,   1);
IPAddress subnet_mask(255, 255, 255,   0);



void setup()
{
  WiFi.hostname("SmartThermostat-V1.7");
  WiFi.mode(WIFI_STA);
  // Debug console
  //Serial.begin(9600);
  WiFi.config(arduino_ip, gateway_ip, subnet_mask);

  dht.begin();
  yield();
  h = dht.readHumidity();
  t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
  yield();
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 0, " CONNECTING ...");   
  display.display();
  yield();
//  Blynk.begin(auth, ssid, pass);
  yield();
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 8442);
  //Or for local server:
  Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,3), 8080);


  // Setup a function to be called every second
  timer.setInterval(5000L, sendTemp);
  yield();
  //timer.setInterval(5000L, TempCompare);
  timer.setInterval(10000L, displayData); 
  yield();
  timer.setInterval(2000L, timesync); 
  yield();  
  timer.setInterval(10000L, connectionstatus);
  yield();  
  
  pinMode (relay, OUTPUT);
  pinMode (upPin, INPUT);
  pinMode (downPin, INPUT);
  pinMode (DHTPIN, INPUT);


  
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
      yield();
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    yield();
  });
  ArduinoOTA.setHostname("Smart Thermostat 1.7"); // OPTIONAL NAME FOR OTA
  yield();
  ArduinoOTA.begin();
  yield();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void loop()
{
  yield();
  ArduinoOTA.handle();
  yield();
yield();
  Blynk.run();
yield();
  dispTempSet = tempset;
yield();
  timer.run();
yield();
  ButtonsUpDown();
yield();

yield();  
  if (interval == 1)
  {
    ledInterval.on();
    yield();
  }
  else
  {
    ledInterval.off();
  yield();  
  }

}



BLYNK_CONNECTED(){
  Blynk.syncVirtual(V10, V11, V13, V12, V16,V14,V19);  // synk on connection
yield();
}

void timesync()
{
    Blynk.syncVirtual(V12);
    rtc.begin();
    yield();
}


void connectionstatus()
{
//  result = Blynk.connected();
//  result = Blynk.connect(60);
  connection = Blynk.connected();
  if (connection == 0)
  {
      connectionattempts ++;
      Serial.println();
      Serial.print("connectionattempts");
      Serial.print(connectionattempts);
      Serial.println();
      yield();
      display.init();
      display.clear();
      display.flipScreenVertically();
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_10);
      display.drawString(64, 0, " CONNECTING ...");   
      display.display();
      yield();
  }
  else 
  {
    connectionattempts = 0;
  }
  
  if (connectionattempts == 5)
  {
      ESP.restart();  
  }
}

BLYNK_WRITE(V12) {
  TimeInputParam t(param);
 
  String currentTime = String(hour()) + ":" + minute() + ":" + second();
  String currentDate = String(day()) + " " + month() + " " + year();
  yield();
  Serial.print("Current time: ");
  Serial.print(currentTime);
  Serial.print("   ");
//  Serial.print(currentDate);
//  Serial.println();
  
    Serial.print(String(" Start: ") +
                   t.getStartHour() + ":" +
                   t.getStartMinute() + ":" +
                   t.getStartSecond());
    Serial.print(String("  Stop:") + 
                   t.getStopHour() + ":" +
                   t.getStopMinute() + ":" +
                   t.getStopSecond());
    if(interval== 1)
    {
      Serial.print(" = > Started!");               
    }
    else
    {
      Serial.print(" = > Stopped");
    }
                          
    
    Serial.println();  Serial.println();
    

    //--> Starting hour is the same as the ending hour. 
    if (t.getStartHour() == hour() && hour() == t.getStopHour())   // hours match.check the minutes
    {
      Serial.println("The Hour is Matching.Check the minutes");
      if (t.getStartMinute() <= minute() && t.getStopMinute() > minute() )
      {
        Serial.println("The start/end Hour is the same, start<Minutes<end ");
        interval = 1;
      }
      if (t.getStartMinute() > minute() )
      {
        Serial.println("Minutes are not matching for the interval, Yet.");
        interval = 0;        
      }
      if (t.getStopMinute() < minute() )
      {
        Serial.println("minutes>Stop Interval");
        interval = 0 ;
      }
    }
    // <--- Starting hour is the same as the ending hour. 
    yield();

    // Stop time matches the current hour. Check the minutes.
    if (t.getStopHour() == hour() ) 
    {
      if ( t.getStopMinute() <= minute()  ) 
      {
        Serial.println("StopTime is passed");
        interval = 0;
      }
      if (t.getStopMinute() > minute() && minute() > t.getStartMinute() )
      {
        Serial.println("During the interval. StartMinute<Minute<StopMinute");
        interval= 1;
      }
    }
    if (t.getStopHour() == t.getStartHour() && t.getStopHour() != hour() )
    {
      Serial.println("The Interval is <1H but the current Hour does not match the interval");
      interval = 0;
    }


    // ---> StartHour > StopHour - It ends next day Eg: 15:00 - 14:00 , 06:00 - 05:00 , 06:00 - 01:00
    if (t.getStartHour() > t.getStopHour() ) 
    {
      Serial.println("StartHour > StopHour - It ends next day ");
      if (t.getStartHour() == hour())
      {
        Serial.println("StartHour=Hour - During the interval.Check the minutes");
        if (t.getStartMinute() <= minute() && minute() <= t.getStopMinute() )
        {
          Serial.println("StartHour=Hour StartMinute<=Minute<=StopMinute");
          interval = 1;
        }
        if (t.getStartMinute() > minute() || minute() > t.getStopMinute() )
        {
          Serial.println("StartHour=Hour StartMinute>Minute or Minute>EndMinute");
          interval = 0;
        }
      }
      // eg: interval: 15:00 - 3:00 
      if (t.getStartHour() > hour() && hour() > t.getStopHour() )
      {
        Serial.println("current H>StartH && CurrentH > StopH");
        interval = 0;
      }
      if (t.getStartHour() > hour() && hour() == t.getStopHour() )
      {
        Serial.println("StartH>Hour && Hour==StopH , check minutes");
        if (t.getStopMinute() <= minute() )
        {
          Serial.println("Few minutes over the given interval. Stop!");
          interval = 0;
        }
        else 
        {
          Serial.println("Last Hour.Check the minutes");
          interval = 1;
        }
      }
      yield();
      if(t.getStartHour() > hour() && hour() < t.getStopHour() )
      {
        Serial.println("StartH>H, H<StopH - During the given interval");
        interval = 1;
      }

      //during the interval StartH<H , H>StopH
      if (t.getStartHour() < hour() && hour() > t.getStopHour() )
      {
        Serial.println("during the interval StartH<H , H>StopH");
        interval = 1;
      }

    }
    // ---> StartHour < StopHour
    
    //daca ora de inceput este mai mica decat ora de sfarsit atunci intervalul se incheie in aceeasi zi
    else if (t.getStartHour() < t.getStopHour() ) 
    {
      Serial.println("Ora de sfarsit se incheie in aceeasi zi");
      if (t.getStartHour() < hour() && t.getStopHour() > hour() )  // ora este mai mare decat ora starttime dar mai mica decat stoptime.
      {
        Serial.println("Se afla in intervalul de ore, nu in aceeasi ora ed sfarsit, deci nu e nevoie sa verific minutele");
        interval= 1;
      }

      if (t.getStartHour() == hour() )
      {
        Serial.println("Valoarea orei se potriveste cu intervalul, verifica si minutele");
        if (t.getStartMinute() <= minute())
        {
          Serial.println("Se potrivesc si minutele. in interval");
          interval=1;  
        }
        else 
        {
          Serial.println("Minutele nu se potrivesc, In afara intervalului@");
          interval = 0;
        }
      }
      if (t.getStartHour() > hour() )
      {
        Serial.println("Inca nu s-a apropiat intervalul.In afara intervalului");
        interval = 0;
      }
      if (t.getStopHour() == hour())
      {
        Serial.println("Ultima ora din interval.verifica minutele");
        if (t.getStopMinute() <= minute() )
        {
          Serial.println("A depasit minutele.In afara intervalului!");
          interval = 0;
        }
        else if (t.getStopMinute() > minute() )
        {
          Serial.println("Nu au depasit minutele.In interval");
          interval = 1;
        }
      }
      
    }


yield();
  Serial.println();
}



void displayData() {
    dispTempSet = tempset;  
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    if (connection == 0){
      display.drawString(64, 0, "Disconnected . . .");        
    }
     else {
      if (scheduled == 1)
      {
        if (interval == 1)
        {
          display.drawString(0, 0, "Set:" + dispTempSet + "°C Scheduled-isTimeTo...");  
        }
        else 
        {
          display.drawString(0, 0, "Set:" + dispTempSet + "°C Scheduled-notYet...");            
        }
      yield();
      }
      else 
      {
        display.drawString(0, 0, "Set:" + dispTempSet + "°C * AllwaysOn");   
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
  Blynk.syncVirtual(V12, V16,V14,V19);
  yield();
  h = dht.readHumidity();
  t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
  yield();
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  display_temp = t;
  display_humid = h;
  Serial.println(display_temp);
  Serial.println(display_humid);
  
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V10, t); //temperature on virtual pin V5
  Blynk.virtualWrite(V11, h); //humidity on virtual pin V6
yield();
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
  else {
     TempCompare();  
     Serial.println("Thermostat is running non stop");   
      yield();
  }
  
 Blynk.virtualWrite(V13, tempset);       // update tempset at the same time with the temperature   
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
  Blynk.virtualWrite(V13, tempset);
  yield();
  Serial.println(param.asInt());  
  Serial.println(stepperValue);  
}


BLYNK_WRITE(V16)   // ON = scheduled   - OFF = manual(check temperature allways)
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


void ButtonsUpDown()
{
   /* Tratează cazul când s-a apăsat up pentru incrementarea tempset. */
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

  /* Tratează cazul când s-a apăsat down pentru decrementarea tempset. */
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

void TempCompare()
{
  yield();
   tempset2 = tempset-1;   
          // incepe din starea
          // HEATING = 0
          // STOPPED = 0
            
          if (HEATING == 0 && STOPPED == 0){
            if (tempset > t){
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
            if (t > tempset){ // prima oprire a termoreleului
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
            if (t < tempset2){ // daca temperatura actuala A SCAZUT cu cel putin un grad fata de cea pe care a avut-o cand s-a oprit abi atunci incepe sa incalzeasca
              HeatOn();
              //Blynk.setProperty(V5, "color", "red");                             
              STOPPED = 0;
              Serial.println("Temp dropped by one deg.Start heating again");
            }
            else {
              HeatOff();
              Serial.println("Keep it off til it drops by one degree");              
            }
          }
}

>>>>>>> 4ca88ee5c4cea01be8d86b34548779efab4a8635


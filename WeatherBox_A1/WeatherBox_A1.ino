
#include <ArduinoJson.h>
#include "Controller.cpp"
#include "Lights.cpp"

#define sizeOfBuf 512

unsigned int timeoutOver;
unsigned int lastFetch;
unsigned int lastActuation;
#define fetchTime_ms 5000
//unsigned int fetchTime_ms = 5000;//900000; //15 min = 900,000 mSec
#define actuationTime_ms 15000
//unsigned int actuationTime_ms = 15000; //15 sec = 15,000 mSec

short lastWeatherCode;

//Set Up Actuation
Controller controller;
Lights lights;

//Set Up ESP 
char espBuf[sizeOfBuf];  //buffer for new Rx from ESP chip
StaticJsonBuffer<sizeOfBuf> jsonBuffer;


void setup() 
{ 
  pinMode(13, OUTPUT);    //our debug/error pin
  lastWeatherCode = 800;  //clear weather
  
  //Init Serial Port for communication to ESP chip
  Serial.begin(115200);
  
  //Turn off all actuators
  controller.allOff();
  lights.off();

//  for(int i = 0; i < 5; i++)
//        {
//          digitalWrite(13, HIGH);
//          delay(200);
//          digitalWrite(13, LOW);
//          delay(100);
//        }
    //do first case manually
//    doESPStuff(1);
    doESPStuff('$');

  //**Now we can actually begin normal operation**

  //Start System Time
  lastFetch = millis();
  lastActuation = millis();
  delay(1000);
}

void loop() 
{
//  //check if time to actuate current JSON
//  if(millis() - lastActuation >= actuationTime_ms)
//  {
//    setWeather(String(lastWeatherCode));
////    Serial.println("^Used old data");
//    lastActuation = millis();
//  }
//  
//  //check if time to fetch new JSON
//  if(millis() - lastFetch >= fetchTime_ms)
//  {
//    //1 is the command to get the latest JSON
//    //doESPStuff(1);
//    doESPStuff('$');
//    lastFetch = millis();
//  }
  demonstration();
}

void doESPStuff(char _cmd)
//void doESPStuff(int _cmd)
{
    int ret1 = sendESPCmd(_cmd);
    if(!ret1)  //want this to return 0
    {
      
      //Parse JSON & Set Weather
      int ret2 = parseESPJson();
      if(ret2 != 0) //want this to return 0
      {  
        //Error -- could not parse resp -- flash onboard LED depending on error
//        for(int i = 0; i < ret2*4; i++)
//        {
//          digitalWrite(13, HIGH);
//          delay(100);
//          digitalWrite(13, LOW);
//          delay(100);
//        }
      }
      
    }
//    else
//    {
//      //Error -- could not send command -- flash onboard LED 
//      for(int i = 0; i < ret1*3; i++)
//      {
//        digitalWrite(13, HIGH);
//        delay(100);
//        digitalWrite(13, LOW);
//        delay(100);
//      }
//    }
}



//int sendESPCmd(int _cmd)
int sendESPCmd(char _cmd)
{
  int count = 0;
  
  //Reset ESP Rx buffer
  memset(espBuf, 0, sizeOfBuf);
  jsonBuffer.clear();
 
  //cmd 1 = fetch JSON
  Serial.print(_cmd);  //Send ESP chip a command to req from OpenWeatherAPI

  while(!Serial.available())
  {
    
  }
  
//  if(Serial.available())
  if(1)
  {
    do
    {
      while(Serial.available() > 0)
      {  
        espBuf[count] = Serial.read();
//        Serial.print(espBuf[count]);
        timeoutOver = millis()+200; //allow 200mSec before timeout/EOT
        count++;
      }
    }
    while(millis()<timeoutOver);
    //espBuf[count] = '\0'; //Signifies end
    
    Serial.print("\nGot ");
    Serial.print(count);
    Serial.println(" bytes");
    for(int i = 0; i < count; i++)
    {
      if(espBuf[i] == '$')
      {
        Serial.print("!");
      }
      else
      {
        Serial.print(espBuf[i]);
      }
      
    }
    Serial.println("\n");
  }
  else
  {
    Serial.println("Serial not available");
    return 1; 
  }
  return 0; //success
}

int parseESPJson()
{
  JsonObject& root = jsonBuffer.parseObject(espBuf);

  if(!root.success())
  {
    Serial.println("Error 1 in parseESPJson");
    return 1; //error -- JsonObject class could not parse object
  }

  if(root["cod"] == 200)
  {
    //For OpenWeatherAPI, the "cod" internal parameter should be 200 
    JsonObject& weather = root["weather"][0];
    String weatherId = weather["id"];

    if(setWeather(weatherId))
    {
      //success
      return 0;
    }
    else
    {
      Serial.println("Error 2 in parseESPJson");
      return 2; //error 
    }
  }
  else
  {
    Serial.println("Error 3 in parseESPJson");
    return 3; //error -- wrong internal parameter
  }
}


int setWeather(String _wthrID)
{
  int weather = atoi(&_wthrID[0]);
  lastWeatherCode = weather;
  
  if(weather >= 200 && weather < 300)
  {
    //thunderstorm
    storm(10);
  }
  else if(weather >= 300 && weather < 400)
  {
    //drizzle (lumped in with "light rain")
    rain();
  }
  else if(weather >= 500 && weather < 600)
  {
    //rain
    rain();
  }
  else if(weather >= 700 && weather < 800)
  {
    //atmospheric conditions
    if(weather == 701 || weather == 721)
    {
      lowFog();
    }
    else if(weather == 741 || weather == 781)
    {
      highFog();
    }
    
  }
  else if(weather > 800 && weather < 900)
  {
    //clouds
    lowFog();
  }
  else if(weather >= 900 && weather < 1000)
  {
    //extreme weather
    storm(10);
  }
  else if(weather == 800)
  {
      //clear
      sunrise();
  }
  else
  {
    //unknown weather ID -- error
    return 0;
  }
  
  Serial.print("Successfully set weather to: ");
  Serial.println(weather);
  return 1; //success
}









/* SUNRISE */
void sunrise() {
  highFog();
  delay(5000);
  controller.allOff();
  delay(100);
  lights.sunrise();
}

/* STORM */
void storm(int seconds) {
  lights.set(0,0,30);
  rain();

  lowFog();
  delay(2000);
  
  for (int i = 0; i < seconds-1; i++) {

    if (i % 2 == 1) {
      lowFog();
    } else {
      highFog();
    }
    lights.flash(255,255,255);
    delay(900);
  }

}

/* RAIN */
void rain() {
  controller.pumpOn();
}

void rain(int redV, int greenV, int blueV) {
  controller.pumpOn();
  lights.on(redV, greenV, blueV);
}


/* LOW FOG */
void lowFog(int _time, int redV, int greenV, int blueV) {
  controller.misterOn();
  lights.on(redV, greenV, blueV);

  for (int i = 0; i < _time/200; i = i + 200) {
    controller.fanOn();
    delay(200);
    controller.fanOff();
  }
  
}

void lowFog() {
  controller.misterOn();
  controller.fanOn();
  delay(200);
  controller.fanOff();
}

void lowFog(int redV, int greenV, int blueV) {
  controller.misterOn();
  lights.on(redV, greenV, blueV);
}


/* HIGH FOG */
void highFog() {
  controller.misterOn();
  controller.fanOn();
}

void highFog(int redV, int greenV, int blueV) {
  controller.misterOn();
  controller.fanOn();
  lights.on(redV, greenV, blueV);
}

void demonstration() {

  // Lights
//  byte rate = 50;
//  lights.fadeInto(255,0,0,rate); // red
////  lights.fadeInto(255,140,0,rate); // orange
////  lights.fadeInto(255,255,0,rate); // yellow
//  lights.fadeInto(0,255,0,rate); // green
//  lights.fadeInto(0,0,255,rate); // blue
////  lights.fadeInto(75,0,130,rate); // indigo
////  lights.fadeInto(238,130,238,rate); // violet
////  lights.fadeInto(199,21,133,rate); // pink
//
//  lights.set(255,0,0); // red
//  delay(rate);
//  lights.set(255,140,0); // orange
//  delay(rate);
//  lights.set(255,255,0); // yellow
//  delay(rate);
//  lights.set(0,255,0); // green
//  delay(rate);
//  lights.set(0,0,255); // blue
//  delay(rate);
//  lights.set(75,0,130); // indigo
//  delay(rate);
//  lights.set(238,130,238); // violet
//  delay(rate);
//  lights.set(199,21,133); // pink
//  delay(rate);

//  lights.off();
  delay(1000);
  
  // Mister
  lowFog(5000, 255, 255, 255);

  // Mister and Fan
  highFog(255, 255, 255);
  delay(3000);

  controller.allOff();
  delay(10);
  
  // Pump
  rain();
  delay(5000);

  storm(10);

  lights.fadeInto(255,255,255,500);
}

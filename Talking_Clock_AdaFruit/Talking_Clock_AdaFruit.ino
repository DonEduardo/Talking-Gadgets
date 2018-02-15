/* 
 Talking Clock/Temperature/Light Device
 
 by Don Eduardo
 
 Receives a struct of tempereture, voltage, and light from remote 
 device and updates display. When button pressed, it will speak those
 values. 
 
 Voice is recorded and can be the voice of a child reading values
 to a grandparent for instance. Makes a great custom gift for a
 family member. 

 See video here: https://youtu.be/0awTu3DbDjg
 
 

/*-----( Import needed libraries )-----*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RF24.h>
#include "RTClib.h"
#include <Adafruit_VS1053.h>
#include <SD.h>


//*** LCD Settings to use over I2C
//


//LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);



/*****    RF24 RADIO Settings /
/*-----( Declare Constants and Pin Numbers )-----*/
#define CE_PIN   9
#define CSN_PIN 10

RF24 radio(CE_PIN, CSN_PIN); // Create a Radio

byte addresses[][6] = {"1Node"}; // Create address for 1 pipe.
typedef struct {
  uint64_t  id;
  float  temperature;
  uint16_t  lightlevel;
  float  voltage;
  uint16_t  reserved[1];
  uint32_t  counter;
} 
TS_message;

TS_message sensorPayload;




//*****   Settings for the Real Time Clock Module /
RTC_DS3231 rtc;
DateTime lastPacketTime; 





//************ VS1053 Settings 
#define CLK 13       // SPI Clock, shared with SD card
#define MISO 12      // Input data, from VS1053/SD card
#define MOSI 11      // Output data, to VS1053/SD card
// Connect CLK, MISO and MOSI to hardware SPI pins. 
// See http://arduino.cc/en/Reference/SPI "Connections"

// These are the pins used for the breakout example
#define BREAKOUT_RESET  7      // VS1053 reset pin (output)
#define BREAKOUT_CS     5     // VS1053 chip select pin (output)
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output)
// These are the pins used for the music maker shield
//#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
//#define SHIELD_CS     7      // VS1053 chip select pin (output)
//#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = 
  // create breakout-example object!
  Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, CARDCS);
  // create shield-example object!
  //Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

//**** Misc Config
int loopcount=0;
#define buttonPin 0
#define buttonAnalog A0


void setup()   /****** SETUP: RUNS ONCE ******/
{

  uint8_t result; //result code from some function as to be tested at later time.


  Serial.begin(115200);
  pinMode(buttonAnalog, INPUT_PULLUP); // sets analog pin for multiple buttons


  // LCD Setup

  lcd.begin();
  lcd.home(); // go home
  lcd.print("Talking Clock");

  // RF24 Radio Setup
  Serial.println("Nrf24L01 Receiver Starting");
  radio.begin();
  radio.setPALevel( RF24_PA_MAX ) ; 
  radio.setDataRate( RF24_250KBPS ) ; 
  radio.setChannel(80);  // Above most Wifi Channels
  radio.openReadingPipe(1, addresses[0]); // Use the first entry in array 'addresses' (Only 1 right now)
  // 8 bits CRC
  //radio.setCRCLength( RF24_CRC_8 ) ; 
  radio.startListening();



  // *** Music Player Setup
  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));
  
  SD.begin(CARDCS);    // initialise the SD card
  
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(5,5);

  //create a string with the filename
  char trackName[] = "track001.mp3";

  //result = musicPlayer.playFullFile("track001.mp3");

  // test and or welcome message
  result = musicPlayer.playFullFile("deg.mp3");


  //**** Real Time Clock Setup
  rtc.begin();
  // Set to true to manually set clock to a time
  if (false) // !rtc.isrunning()) 
  {
   rtc.adjust(DateTime(2018, 2, 4, 22, 04, 0));
  }

  lastPacketTime = rtc.now();
}//--(end setup )---


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{
  char tempF[6]; 

  if ( radio.available() )
  {
    Serial.println("Got radio signal...");
    // Read the data payload until we've received everything
    //   bool done = false;
    //  while (!done)
    while(radio.available())
    {
      // Fetch the data payload
      radio.read( &sensorPayload, sizeof(sensorPayload) );
      Serial.print("\n\rTemp = ");
      Serial.print(sensorPayload.temperature);
      Serial.print(" Light = ");      
      Serial.print(sensorPayload.lightlevel);
      Serial.print(" Voltage = ");      
      Serial.println(sensorPayload.voltage);
      lcd.setCursor(0,1); //Go to second line of the LCD Screen
      lcd.print(' ');
      dtostrf(sensorPayload.temperature, 5, 1, tempF);

      lcd.print(tempF);
      lcd.print((char)223);
      lcd.print("F ");

      lcd.print("V:");
      lcd.print(sensorPayload.voltage);
      showTime();
      announcer();
      lastPacketTime = rtc.now();

      //delay(3000);
    }
  }
  else
  {    
    Serial.print(".");
    loopcount++;
    if (loopcount > 9){
      Serial.println("\n\rWaiting for packet");
      showTime();
      loopcount=0;
      if (rtc.now().unixtime() > (lastPacketTime.unixtime() + 300))
      {
        lcd.setCursor(1,1);
        lcd.print("No Update Lately");
      }

    }
    int aButton=readButtons(buttonPin);
    //Serial.println(a);
    if(aButton==5)
      announcer();

    //delay(100);

  }



}//--(end main loop )---


int readButtons(int analogPin)
// returns the button number pressed, or zero for none pressed 
// int pin is the analog pin number to read 
{
  int b,c = 0;
  unsigned long sum=0;
  for (byte  i = 0; i < 100; i++)
  {
    sum += analogRead(analogPin);
  }

  //Serial.println(sum);
  c = sum / 100;
  //Serial.println(c);

 // c=analogRead(pin); // get the analog value  
 // Serial.println(c);
  if (c>1000)
  {
    b=0; // buttons have not been pressed
  }   
  else
    if (c>190 && c<200)
    {
      b=1; // button 1 pressed
    }     
    else
      if (c>150 && c<160)
      {
        b=2; // button 2 pressed
      }       
      else
        if (c>100 && c<120)
        {
          b=3; // button 3 pressed
        }         
        else
          if (c>60 && c<70)
          {
            b=4; // button 4 pressed
          }           
          else
            if (c<20)
            {
              b=5; // button 5 pressed
            }
  if(b>0)
  {
  Serial.print("Button Pressed = ");
  //delay(1000);
  Serial.println(b);  
  }       
  return b;
}

void showTime()
{
  DateTime now = rtc.now();
  lcd.setCursor(0,0);
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  lcd.print(now.month(), DEC);
  lcd.print('/');
  lcd.print(now.day(), DEC);
  lcd.print('/');
  lcd.print(now.year(), DEC);

  lcd.print(' ');
  if(now.hour() > 12)
    lcd.print(now.hour() -12);
  else if (now.hour() == 0)
    lcd.print('12');
  else
    lcd.print(now.hour(), DEC);
  lcd.print(':');
  if (now.minute() < 10)
    lcd.print('0');
  lcd.print(now.minute(), DEC);
  lcd.print(' ');
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

}

void forgroundPlay(String fileName)
{
  //Serial.print("filename = " +fileName);
  char trackName[15]; // 8.3 plus some extra, in case :)
  fileName.toCharArray(trackName,15);
  musicPlayer.playFullFile(trackName);

}

void announcer()
{ //  annouces time, temperature, and light
  DateTime now = rtc.now();
  int currhr = now.hour();
  int currmin = now.minute();
  int currsec = now.second();
  if (currhr < 12)
    forgroundPlay("morn.mp3");
  else if (currhr < 17)
    forgroundPlay("anoon.mp3");
  else
    forgroundPlay("geve.mp3");

  playhours(currhr);

  playmins(currmin);

  if (currhr >= 12)
    forgroundPlay("pm.mp3");
  else
    forgroundPlay("am.mp3");

  delay(500);
  playnumber( (int)sensorPayload.temperature);
  forgroundPlay("deg.mp3");
  //  forgroundPlay("faren.mp3");
  delay(500);
  forgroundPlay("lightl.mp3");

  playnumber(lightScaled(sensorPayload.lightlevel));

}


int lightScaled(uint16_t lux) {
  int scale = 0;
  if (lux <= 10) 
    scale = map(lux, 0,10, 0,10);
  else if (lux <= 100)
    scale = map(lux, 11,100,11,20);
  else if (lux <= 400)
    scale = map(lux, 101,400,21,50);
  else if (lux <= 5000)
    scale = map(lux, 401,5000,51,80);
  else if (lux <=20000)
    scale = map(lux, 5001,20000,81,90);
  else
    scale = map(lux, 20001,65535,91,100);

  return scale;
}


void playhours(int hrs)  
{ //' announce hours
  if (hrs == 0)
    playnumber(12);
  else if (hrs < 13)
    playnumber(hrs);
  else
    playnumber(hrs - 12);
}

void playmins(int minns)   
{ // announce minutes
  if (minns == 0)
    forgroundPlay("exact.wav");
  else if (minns < 10) {
    playnumber(0);
    playnumber(minns);
  }
  else
    playnumber(minns);

}


void playsecs(int secs)   
{ //announce seconds
  playnumber(secs);
  forgroundPlay("secs.mp3");
}

void playnumber(int number) 
{
  int tendig, onedig  ;            //generic routine to announce numbers
  if (number < 20)
    forgroundPlay(String(number) + ".mp3");
  else if (number < 100){
    tendig = (int)(number / 10);
    tendig = tendig * 10;

    onedig = number - tendig;
    forgroundPlay(String(tendig) + ".mp3");

    if (onedig != 0)
      forgroundPlay(String(onedig) + ".mp3"); 
  }
  else if (number == 100) {
    forgroundPlay("100.mp3");
  }
  else {
    forgroundPlay("100.mp3");
    number = number - 100;
    tendig = (int)(number / 10);
    tendig = tendig *10;
    onedig = number - tendig;
    forgroundPlay(String(tendig) + ".mp3");
    if (onedig != 0)
      forgroundPlay(String(onedig) + ".mp3");
  }
}
























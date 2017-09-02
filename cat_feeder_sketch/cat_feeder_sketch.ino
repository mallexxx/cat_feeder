#include "U8glib.h"
#include <Wire.h>

#define RTC_I2C_ADDRESS 0x68
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);

#define DIR 5
#define STEP 6
#define ENA 7

#define LED 13

#define ENTER 2

void setTime(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year) {
  // sets time and date data to DS3231
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

void setup() { 
  Wire.begin();
    
  pinMode(ENA, OUTPUT); 
  pinMode(DIR, OUTPUT); 
  pinMode(STEP, OUTPUT);
  digitalWrite(DIR, LOW);
  digitalWrite(STEP, LOW);
  digitalWrite(ENA, HIGH);
  
  pinMode(LED, OUTPUT);
  pinMode(ENTER, INPUT);

  for (int i = 0; i < 100; i++) {
    delay (5);
    digitalWrite(DIR, LOW);
    digitalWrite(STEP, LOW);
    digitalWrite(ENA, HIGH);
  }

//  setTime(0, 34, 16, 5, 2, 9, 2017);
} 

inline uint8_t bcd2dec(uint8_t bcd) {
  return ((bcd / 16) * 10) + (bcd % 16);
}
inline uint8_t decToBcd(byte val) {
  return( (val/10*16) + (val%10) );
}

uint8_t second = 0;
uint8_t minute = 0;
uint8_t hour = 0;
int16_t togo = 1;
uint16_t counter = 0;

void draw() {
  u8g.setRot180();
  u8g.setFont(u8g_font_unifont);
  u8g.setPrintPos(10, 20);
  u8g.print(hour, DEC);
  u8g.print(":");
  if (minute < 10)
    u8g.print("0");
  u8g.print(minute, DEC);
  u8g.print(":");
  if (second < 10)
    u8g.print("0");
  u8g.print(second, DEC);

  u8g.setPrintPos(10, 40);
  u8g.print(counter, DEC);
  
  u8g.setPrintPos(10, 60);
  if (togo == 0) {
    u8g.print("Feeding!");
  } else {
    u8g.print(togo / 60, DEC);
    u8g.print(":");
    if ((togo % 60) < 10)
      u8g.print("0");
    u8g.print(togo % 60, DEC);
    u8g.print(" to go");
  }
}

void feed() {
  digitalWrite(ENA, LOW);
  
  for(uint8_t i=0; i < 5; i++){   
    for (uint8_t f=0 ; f<90; f++) {
      digitalWrite(DIR, LOW);
            
      digitalWrite(STEP, LOW);
      delay(5);
      
      digitalWrite(STEP, HIGH);
      delay(5);
    }
    for (uint8_t f=0 ; f<60; f++) {
      digitalWrite(DIR, HIGH);
       
      digitalWrite(STEP, LOW);    
      delay(5);
      digitalWrite(STEP, HIGH);
      delay(5);
    }
  }
  digitalWrite(ENA, HIGH);
}

void loop() {
    Wire.beginTransmission( RTC_I2C_ADDRESS );
    Wire.write(0);
    Wire.endTransmission();
  
    Wire.requestFrom(RTC_I2C_ADDRESS, 3);
    
    second = bcd2dec( Wire.read() );
    minute = bcd2dec( Wire.read() );
    hour = bcd2dec( Wire.read() );

    // 8:00 = 480min
    // 17:00 = 1020min
    // 23:00 = 1380min
    togo = min((480 - (int16_t)hour*60-(int16_t)minute+1440)%1440, 
        min((1020 - (int16_t)hour*60-(int16_t)minute+1440)%1440, (1380 - (int16_t)hour*60-(int16_t)minute+1440)%1440));

    u8g.firstPage();  
    do {
      draw();
    } while( u8g.nextPage() );

    if (second == 0 && togo == 0)
      feed();
    delay(250);
}
  




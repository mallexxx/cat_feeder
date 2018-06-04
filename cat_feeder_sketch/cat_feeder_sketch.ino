#include "U8glib.h"
#include <Wire.h>
#include <TMC2130Stepper.h>

#define RTC_I2C_ADDRESS 0x68
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);

#define BUTTON 5
#define CS 6
#define ENA 7
#define DIR 8
#define STEP 9

TMC2130Stepper driver = TMC2130Stepper(ENA, DIR, STEP, CS);

uint8_t second = 0;
uint8_t minute = 0;
uint8_t hour = 0;
int16_t togo = 1;
uint16_t counter = 0;
uint8_t cnt = 0;
uint8_t data= 0;
char cmd;

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
  digitalWrite(ENA, HIGH);
  
  Serial.begin(115200);
  Serial.print("Starting...");
  
  Wire.begin();
    
  pinMode(BUTTON, INPUT);
  
  driver.begin();
  driver.rms_current(1300);
  driver.stealthChop(1);

  Serial.println(" Ready!");
} 

inline uint8_t bcd2dec(uint8_t bcd) {
  return ((bcd / 16) * 10) + (bcd % 16);
}
inline uint8_t decToBcd(byte val) {
  return( (val/10*16) + (val%10) );
}

void draw() {
//  u8g.setRot180();
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

void feed(uint8_t cnt) {
  Serial.print("Feeding (");
  Serial.print(cnt, DEC);
  Serial.println(")");
  
  digitalWrite(ENA, LOW);

  for(uint8_t i=0; i < cnt; i++){
    driver.shaft_dir(0);
    for (uint16_t f=0 ; f<9000; f++) {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(5);
      digitalWrite(STEP, LOW);
      delayMicroseconds(5);
    }
    driver.shaft_dir(1);
    for (uint16_t f=0 ; f<16000; f++) {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(5);

      digitalWrite(STEP, LOW);
      delayMicroseconds(5);
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

    if (Serial.available()) {
      data = Serial.read();
      cmd = data;
      
      if (Serial.available()) {
        data = Serial.read();
        
        if ('0' <= data && data <= '9') {
          cnt = data - '0';
          
          if (Serial.available()) {
            data = Serial.read();

            if ('0' <= data && data <= '9') {
              cnt = cnt * 10;
              cnt += data - '0';
            }
          }
        } else {
          cnt = 1;
        }
      } else {
        cnt = 1;
      }
      switch (cmd) {
      case 'f':
        // Feed:
        feed(cnt);
        break;
        
      case 'm':
        if (cnt <= 60) {
          // Set minute
          minute = cnt;
          Serial.print("Setting minutes to ");
          Serial.println(minute, DEC);
          cnt=255;
        } else {
          break;
        }
        
        // fallthrough
      case 'h':
        if (cnt <= 24) {
          // Set hour
          hour = cnt;
          Serial.print("Setting hours to ");
          Serial.println(hour, DEC);
          cnt=255;
        }// else {
          // fallthrough
        //}
        
      case 's':
        if (cnt <= 60) {
          // Set second
          second = cnt;
          Serial.print("Setting seconds to ");
          Serial.println(second, DEC);
          cnt=255;
        }
      
        // Update time
        setTime(second, minute, hour, 7, 2, 6, 18);
        
      // fallthrough
        
      case 't':
        Serial.print("The current time is: ");
        Serial.print(hour, DEC);
        Serial.print(":");
        if (minute < 10)
          Serial.print("0");
        Serial.print(minute, DEC);
        Serial.print(":");
        if (second < 10)
          Serial.print("0");
        Serial.println(second, DEC);
        break;
        
      }
      cnt = 0;
    }
    if (digitalRead(BUTTON)) {
        if (!cnt) {
            cnt = 1;
            counter++;

            togo = 0;
            u8g.firstPage();  
            do {
              draw();
            } while( u8g.nextPage() );
            
            feed(1);
        }
    } else {
        cnt = 0;
    }

    // 8:00 = 480min
    // 17:00 = 1020min
    // 23:00 = 1380min
    togo = min((480 - (int16_t)hour*60-(int16_t)minute+1440)%1440, 
        min((1020 - (int16_t)hour*60-(int16_t)minute+1440)%1440, (1380 - (int16_t)hour*60-(int16_t)minute+1440)%1440));

    u8g.firstPage();  
    do {
      draw();
    } while( u8g.nextPage() );

    if (second == 0 && togo == 0) {
        counter++;
        feed(16);
    }
    delay(250);
}
  




/* 

WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING 

This code requires significant fiddling inside of the Arduino distribution to
free up enough RAM to run without crashing.

1) Shrink the I2C twi BUFFER_SIZE from 32 to 8 in the following file:
[Arduino Path]/Resources/Java/libraries/Wire/utility/twi.h 

2) Shrink the I2C Wire BUFFER_LENGTH from 32 to 8 in the following file:
[Arduino Path]/Resources/Java/libraries/Wire/Wire.h

3) Shrink the Hardware Serial buffers from 16 to 4 in the following file:
[Arduino Path]/Resources/Java/hardware/arduino/cores/arduino/HardwareSerial.cpp
41c41
<   #define SERIAL_BUFFER_SIZE 4
---
>   #define SERIAL_BUFFER_SIZE 16
43c43
<   #define SERIAL_BUFFER_SIZE 4
---
>   #define SERIAL_BUFFER_SIZE 64

*/

#include <EEPROM.h>        // stock header
#include <avr/pgmspace.h>  //AVR library for writing to ROM
#include "Charliplexing.h"
#include <Wire.h>          // stock header
#include "RealTimeClockDS1307.h"
#include "tinyfont.h" 
#include <stdio.h>

#include <dht11.h> // I'll copy this over eventually
dht11 DHT11;
#define DHT11PIN 17

#include <OneWire.h> // from the Arduino Playground
#define ONE_WIRE_PIN 16
OneWire ds(ONE_WIRE_PIN);  // on pin 10                                                                                                                             
byte ds_addr[8];
#define ONEWIRE_SEARCH 1

#define SET_BUTTON_PIN 14
#define INC_BUTTON_PIN 15

#define COLS 10 // usually x
#define ROWS 11 // usually y

// set to true to enable serial monitor - warning: uses ~2k of progmem and just
// shy of 256 bytes of ram.
#define _DEBUG_ true

// Do you want the cells to dim as they age or stay the same brightness?
#define AGING true

// Do you want the world to be a toroid?
#define TOROID 1

// How often should it display a clock?
#define CLOCK_EVERY 5000

uint8_t max_brightness=31;

byte world[COLS][ROWS][2]; // Create a double buffered world.
byte frame_log[COLS][ROWS];

uint8_t hours;
uint8_t minutes;

boolean isSettingHours      = true;
boolean isSettingMinutes    = false;
boolean isSettingBrightness = false;

#define BOUNCE_TIME_BUTTON  300   // bounce time in ms for the menu button - too short and every click is an adventure, too long and it's ponderous.

// last time a button was pushed; used for debouncing;
volatile unsigned long lastButtonTime = 0;

// indicates that one of the menu options (0..MAX_OPTION) is currently displayed;
boolean isSettingTime = false;

// when alternating between displaying quotes and time, start with the time;
boolean isDisplayingTime  = true;
boolean isDisplayingQuote = false;

// buffer for time display;
char timeBuffer[]   = "12:34";
char tempnhum[] = "tttt< hhh;";

//--------------------------------------------------------------------------------
void setup() {
  pinMode(SET_BUTTON_PIN, INPUT_PULLUP); // A0
  pinMode(INC_BUTTON_PIN, INPUT_PULLUP); // A1
  pinMode(DHT11PIN, INPUT);

  if(_DEBUG_) { Serial.begin(115200); }

  // Reset the OneWire bus before LedSign::Init monkeys with ports. (possibly before SetBrightness?)
  ds.reset();
  if ( !ds.search(ds_addr)) {
    ds.reset_search();
  }

  // Read the saved defaults
  EEReadSettings();

  // Initialize the screen  
  LedSign::Init(GRAYSCALE);
  LedSign::SetBrightness(max_brightness);

  // Initialize the I2C bus
  Wire.begin();

  // let the clock seed our randomizer - seems to work
  RTC.readClock();
  randomSeed(RTC.getSeconds()); // is actually less random than it should be.  hrm.
  updateTimeBuffer();
}

void loop() {
  // check SET button;
  if (digitalRead(SET_BUTTON_PIN) == 0) {
    // "Set time" button was pressed;
    processSetButton();
  }
  
  // check INC button;
  if (isSettingTime) {
    if (digitalRead(INC_BUTTON_PIN) == 0) {
      // "Inc time" button was pressed;
      processIncButton();
      // save the new time;
      setTime();
    }
  }
  
  // display the menu option for 5 seconds after menu button was pressed;
  if ((lastButtonTime > 0) && (millis() - lastButtonTime < 5000)) {
    return; // avoids hitting the next conditional.
  }
  
  // return the main mode if no button was pressed for 5 seconds;
  if (isSettingTime) {
    // just finished setting up the time;
    isSettingTime = false;
    
    updateTimeBuffer();
    
    resetDisplay();

    // This is smart enough to not keep writing the same value to the eeprom
    EESaveSettings();
  }
  else {
    
    //clear the current world of whatever had been in it.
    for(uint8_t y = 0; y < ROWS; y++) { for(uint8_t x = 0; x < COLS; x++) { world[x][y][0] = 0; world[x][y][1] = 0; } }
    
    unsigned long now=millis();
    
    switch(random(10)) {
    case 0:
      Logo(1000);
      break;
    case 1 ... 3:
      Rain(now,5000);
      break;
    case 4 ... 9:
      Life();
      break;
    }

    if(_DEBUG_) { Serial.println("-----"); Serial.println("Back in loop"); }

    // this takes a while, may as well ask for it before a guaranteed second display.
    if(_DEBUG_) { Serial.println("Requesting temp."); }
    RequestDS18B20Temp();

    // Display the time for 3000ms
    if(_DEBUG_) { Serial.println("Updating time"); }
    updateTimeBuffer();
    DisplayTime(3000);
    
    // The brightness routine does strange things to the DHT reading code causing it to timeout more often than I'd like.  So I pin it to the maximum and the reads tend to work out nicer.
    LedSign::Clear();
    LedSign::SetBrightness(127);
    
    // And the additional delay seems to knock out the last one or two errored readings.
    delay(50);
    float ftemp = GetDS18B20Temp();
    int chk = DHT11.read(DHT11PIN);
    LedSign::SetBrightness(max_brightness);
    
    ftemp = (ftemp*1.8) + 32;
    char temperature[5];
    dtostrf(ftemp, -4, 1, temperature);

    if(_DEBUG_) {
      Serial.print(hours, DEC); Serial.print(":");  Serial.println(minutes, DEC);
      Serial.print("Temp: "); Serial.println(temperature);
    }      

    if(chk == 0) {
      int humidity = DHT11.humidity;

      if(_DEBUG_) {
	Serial.print("Humidity: "); Serial.println(humidity, DEC);
      }
      
      sprintf(tempnhum, "%s< %d;", temperature, humidity);
      Banner(tempnhum, 100, random(6));
    }
    else {
      if(_DEBUG_) {
	Serial.println("Humidity: ERR"); 
	Serial.print("Chksum: "); Serial.println(chk);
      }
      
      sprintf(tempnhum, "%s<", temperature);
      Banner(tempnhum, 100, random(6));
    }    
  }
}

//--------------------------------------------------------------------------------
// functions

void EEReadSettings (void) {  // TODO: Detect ANY bad values, not just 255.

  byte detectBad = 0;
  byte value = 255;

  value = EEPROM.read(0);
  if ((value > 49) || (value < 31))
    detectBad = 1;
  else
    max_brightness = value;  // MainBright has maximum possible value of 8.

  if (detectBad) {
    max_brightness = 49;
  }
}

void EESaveSettings (void){
  //EEPROM.write(Addr, Value);

  // Careful if you use  this function: EEPROM has a limited number of write
  // cycles in its life.  Good for human-operated buttons, bad for automation.

  byte value = EEPROM.read(0);
  if(value != max_brightness) {
    EEPROM.write(0, max_brightness);
    if(_DEBUG_) { Serial.println("eesave"); }
  }
  else {
    if(_DEBUG_) { Serial.println("eenosave"); }
  }
}

void Logo(unsigned long runtime) {
  if(_DEBUG_) { Serial.println("-----"); Serial.println("Logo starting"); }
  LedSign::Clear();
  switch(random(2)) {
  case 0: // major fruit company logo
    for(byte g=0; g<=7; g++) {
      LedSign::Set(0,4,g); LedSign::Set(0,5,g); LedSign::Set(0,6,g); LedSign::Set(0,7,g); LedSign::Set(0,8,g); LedSign::Set(1,3,g); LedSign::Set(1,4,g); LedSign::Set(1,5,g); LedSign::Set(1,6,g); LedSign::Set(1,7,g); LedSign::Set(1,8,g); LedSign::Set(1,9,g); LedSign::Set(2,3,g); LedSign::Set(2,4,g); LedSign::Set(2,5,g); LedSign::Set(2,6,g); LedSign::Set(2,7,g); LedSign::Set(2,8,g); LedSign::Set(2,9,g); LedSign::Set(2,10,g); LedSign::Set(3,3,g); LedSign::Set(3,4,g); LedSign::Set(3,5,g); LedSign::Set(3,6,g); LedSign::Set(3,7,g); LedSign::Set(3,8,g); LedSign::Set(3,9,g); LedSign::Set(3,10,g); LedSign::Set(4,1,g); LedSign::Set(4,2,g); LedSign::Set(4,4,g); LedSign::Set(4,5,g); LedSign::Set(4,6,g); LedSign::Set(4,7,g); LedSign::Set(4,8,g); LedSign::Set(4,9,g); LedSign::Set(5,0,g); LedSign::Set(5,1,g); LedSign::Set(5,3,g); LedSign::Set(5,4,g); LedSign::Set(5,5,g); LedSign::Set(5,6,g); LedSign::Set(5,7,g); LedSign::Set(5,8,g); LedSign::Set(5,9,g); LedSign::Set(5,10,g); LedSign::Set(6,0,g); LedSign::Set(6,3,g); LedSign::Set(6,4,g); LedSign::Set(6,5,g); LedSign::Set(6,6,g); LedSign::Set(6,7,g); LedSign::Set(6,8,g); LedSign::Set(6,9,g); LedSign::Set(6,10,g); LedSign::Set(7,3,g); LedSign::Set(7,4,g); LedSign::Set(7,7,g); LedSign::Set(7,8,g); LedSign::Set(7,9,g); LedSign::Set(8,4,g); LedSign::Set(8,7,g); LedSign::Set(8,8,g);
      delay(50);
    }
    delay(runtime);
    for(byte g=7; g>=1; g--) {
      LedSign::Set(0,4,g); LedSign::Set(0,5,g); LedSign::Set(0,6,g); LedSign::Set(0,7,g); LedSign::Set(0,8,g); LedSign::Set(1,3,g); LedSign::Set(1,4,g); LedSign::Set(1,5,g); LedSign::Set(1,6,g); LedSign::Set(1,7,g); LedSign::Set(1,8,g); LedSign::Set(1,9,g); LedSign::Set(2,3,g); LedSign::Set(2,4,g); LedSign::Set(2,5,g); LedSign::Set(2,6,g); LedSign::Set(2,7,g); LedSign::Set(2,8,g); LedSign::Set(2,9,g); LedSign::Set(2,10,g); LedSign::Set(3,3,g); LedSign::Set(3,4,g); LedSign::Set(3,5,g); LedSign::Set(3,6,g); LedSign::Set(3,7,g); LedSign::Set(3,8,g); LedSign::Set(3,9,g); LedSign::Set(3,10,g); LedSign::Set(4,1,g); LedSign::Set(4,2,g); LedSign::Set(4,4,g); LedSign::Set(4,5,g); LedSign::Set(4,6,g); LedSign::Set(4,7,g); LedSign::Set(4,8,g); LedSign::Set(4,9,g); LedSign::Set(5,0,g); LedSign::Set(5,1,g); LedSign::Set(5,3,g); LedSign::Set(5,4,g); LedSign::Set(5,5,g); LedSign::Set(5,6,g); LedSign::Set(5,7,g); LedSign::Set(5,8,g); LedSign::Set(5,9,g); LedSign::Set(5,10,g); LedSign::Set(6,0,g); LedSign::Set(6,3,g); LedSign::Set(6,4,g); LedSign::Set(6,5,g); LedSign::Set(6,6,g); LedSign::Set(6,7,g); LedSign::Set(6,8,g); LedSign::Set(6,9,g); LedSign::Set(6,10,g); LedSign::Set(7,3,g); LedSign::Set(7,4,g); LedSign::Set(7,7,g); LedSign::Set(7,8,g); LedSign::Set(7,9,g); LedSign::Set(8,4,g); LedSign::Set(8,7,g); LedSign::Set(8,8,g);
      delay(50);
    }
    LedSign::Clear();
    delay(200);
    break;
  case 1: // a heart
    for(byte g=0; g<=7; g++) {
      LedSign::Set(0,3,g); LedSign::Set(0,4,g); LedSign::Set(0,5,g); LedSign::Set(0,6,g); LedSign::Set(1,2,g); LedSign::Set(1,7,g); LedSign::Set(2,1,g); LedSign::Set(2,8,g); LedSign::Set(3,1,g); LedSign::Set(3,9,g); LedSign::Set(4,2,g); LedSign::Set(4,10,g); LedSign::Set(5,2,g); LedSign::Set(5,10,g); LedSign::Set(6,1,g); LedSign::Set(6,9,g); LedSign::Set(7,1,g); LedSign::Set(7,8,g); LedSign::Set(8,2,g); LedSign::Set(8,7,g); LedSign::Set(9,3,g); LedSign::Set(9,4,g); LedSign::Set(9,5,g); LedSign::Set(9,6,g);
      delay(50);
    }
    delay(runtime);
    for(byte g=7; g>=1; g--) {
      LedSign::Set(0,3,g); LedSign::Set(0,4,g); LedSign::Set(0,5,g); LedSign::Set(0,6,g); LedSign::Set(1,2,g); LedSign::Set(1,7,g); LedSign::Set(2,1,g); LedSign::Set(2,8,g); LedSign::Set(3,1,g); LedSign::Set(3,9,g); LedSign::Set(4,2,g); LedSign::Set(4,10,g); LedSign::Set(5,2,g); LedSign::Set(5,10,g); LedSign::Set(6,1,g); LedSign::Set(6,9,g); LedSign::Set(7,1,g); LedSign::Set(7,8,g); LedSign::Set(8,2,g); LedSign::Set(8,7,g); LedSign::Set(9,3,g); LedSign::Set(9,4,g); LedSign::Set(9,5,g); LedSign::Set(9,6,g);
      delay(50);
    }
    LedSign::Clear();
    delay(200);
    break;
  }
}
  
void initialize_frame_log() {
  for(uint8_t y=0; y < ROWS; y++) {
    for(uint8_t x=0; x < COLS; x++) {
      frame_log[x][y] = -1;
    }
  }
}

void log_current_frame() {
  for(uint8_t y=0; y < ROWS; y++) {
    for(uint8_t x=0; x < COLS; x++) {
      frame_log[x][y] = world[x][y][0];
    }
  }
}

void set_random_next_frame(void) {
  // blank out the world
  resetDisplay();
  
  uint8_t density = random(40,80);
  for(uint8_t y=0; y<ROWS; y++) {
    for(uint8_t x=0; x<COLS; x++) {
      if(random(100) > density) {
	world[x][y][1] = 7;
	if(_DEBUG_) { Serial.print("X"); }
      }
      else {
	if(_DEBUG_) { Serial.print("."); }
      }
    }
    if(_DEBUG_) { Serial.println(""); }
  }

// A blinker for testing toroidicity
//  world[0][0][1] = 7;
//  world[0][1][1] = 7;
//  world[0][2][1] = 7;

// A glider for testing toroidicity
//  world[1][0][1] = 7;
//  world[2][1][1] = 7;
//  world[0][2][1] = 7;
//  world[1][2][1] = 7;
//  world[2][2][1] = 7;

}

char current_equals_next() {
  uint8_t x, y;
  for(y=0; y<ROWS; y++) {
    for(x=0; x<COLS; x++) {
      if( world[x][y][0] != world[x][y][1] ) {
	return 0;
      }
    }
  }
  return 1;
}

uint8_t next_equals_glider() {
  uint8_t comp1, comp2, comp3, cellcount, glidermatch;
  comp1 = 0;
  comp2 = 0;
  comp3 = 0;
  cellcount = 0;
  glidermatch = 0;

  for(uint8_t y=0; y<ROWS; y++) {
    comp1 = 0;
    for(uint8_t x=0; x<COLS; x++) {
      if(world[x][y][1] >= 1) {
        // VERY IMPORTANT LINE.
        comp1 = comp1 + (1 << x);
        cellcount++;
      }
    }
    // Shift everything over to the "left" (right shifting, I know.  Shut up.) - don't go all the way down to 1 otherwise you get false positives.
    while((comp3 > 2) && (comp2 > 2) && (comp1 > 2)) { comp3 >>= 1; comp2 >>=1; comp1 >>=1; }

    // match for glider fingerprints -- all vertical gliders transform into
    // horizontal gliders during their life, so I only need to match 4 possible
    // orientations
    if((comp1 == 7) && (comp2 == 4) && (comp3 == 2) ||  // DR glider
       (comp1 == 14) && (comp2 == 2) && (comp3 == 4) || // DL glider
       (comp1 == 7) && (comp2 == 1) && (comp3 == 2) ||  // DL glider
       (comp1 == 4) && (comp2 == 2) && (comp3 == 14) || // UL Glider
       (comp1 == 2) && (comp2 == 1) && (comp3 == 7) ||  // UL Glider
       (comp1 == 2) && (comp2 == 4) && (comp3 == 7))    // UR Glider
      { glidermatch++; }

    // rotate everything down the line.
    comp3 = comp2; comp2 = comp1; comp1 = 0;
  }

  // If there's a glider-like shape with more than 5 living cells, it's a false
  // positive.  If it is a glider and a seperate colony, the glider will collide
  // with the colony eventually.
  if((cellcount == 5) and (glidermatch > 0)) { return 1; }
  else { return 0; }
}

uint8_t next_equals_logged_frame(){
  for(uint8_t y = 0; y<ROWS; y++) {
    for(uint8_t x = 0; x<COLS; x++) {
      if(world[x][y][1] != frame_log[x][y] ) {
	return 0;
      }
    }
  }
  return 1;
}

void Life() {
  if(_DEBUG_) { Serial.println("-----"); Serial.println("Life starting"); }
  int frame_number, generation, temp_generation;
  unsigned int curtime, lasttime;
  curtime = 0;
  lasttime = 0;
  frame_number = 0;
  generation = 0;
  temp_generation = 0;

  initialize_frame_log(); // blank out the frame_log world
  
  // flash the screen - ~1000msec
  for(uint8_t y=0; y < ROWS; y++) { for(uint8_t x=0; x < COLS; x++) { world[x][y][1] = 7; } }
  fade_to_next_frame(30);
  delay(300); 
  for(uint8_t y=0; y < ROWS; y++) { for(uint8_t x=0; x < COLS; x++) { world[x][y][1] = 0; } }
  fade_to_next_frame(30);
  delay(300); 

  // draw the initial generation
  set_random_next_frame();
  fade_to_next_frame(30);
  delay(150);

  unsigned long starttime = millis();
  boolean end_before_next_time=false;
  
  while(1) {
    // this should probably get simplified.
    lasttime = curtime;
    curtime = abs(millis());
    int difftime = curtime - lasttime;

    // show the clock every CLOCK_EVERY seconds
    if(abs(millis()) >= starttime + CLOCK_EVERY) {
      if(end_before_next_time == true) {
	for(int f=0; f<500; f++) {
	  draw_frame();
	}
	break;
      }
      delay(150);
      LedSign::Clear();

      noInterrupts();
      RequestDS18B20Temp();
      interrupts();
      
      updateTimeBuffer();
      if(_DEBUG_) {
	Serial.print(hours, DEC); Serial.print(":");  Serial.print(minutes, DEC);
	Serial.print(" at generation "); Serial.println(generation);
      }
      DisplayTime(1000);

      // If we're deep in generations (2 generations/second, roughly), start showing the time again.
      if(temp_generation >= 60) {
	temp_generation = 0;
	LedSign::Clear();
	noInterrupts();
	float ftemp = GetDS18B20Temp();
	interrupts();
	
	if((ftemp < 50.00) && (ftemp > 2)) { // sanity check the resulting data.
	  ftemp=(ftemp*1.8)+32;
	  char temperature[5];
	  dtostrf(ftemp, -4, 1, temperature);
	  
	  if(_DEBUG_) {
	    Serial.print("Temp: "); Serial.println(temperature);
	  }
	  
	  sprintf(tempnhum, "%s<", temperature);
	  Banner(tempnhum, 100, random(6));
	}
	else {
	  Serial.println("Temp Error!");
	}
      }
      
      starttime = millis();
      if (digitalRead(SET_BUTTON_PIN) == 0) {
	// "Set time" button was pressed
	break;
      }
      continue;
    }

    // Log every 20th frame to monitor for repeats
    if( frame_number == 0 ) { 
      log_current_frame(); 
    }
    
    // generate the next generation
    generate_next_generation();

    // Death due to still life
    // if there are no changes between the current generation and the next generation (still life), break out of the loop.

    // this might not work so well with the end_before_next_time flag, since it leads to blank displays.
    if(( current_equals_next() == 1 ) and ( end_before_next_time == false)) {
      if(_DEBUG_) { Serial.print("Died due to still life at generation "); Serial.println(generation); }
      end_before_next_time = true;
    }
    
    // Death due to oscillator
    // If the next frame is the same as a frame from 20 generations ago, we're in a loop.
    if(( next_equals_logged_frame() == 1 ) and (end_before_next_time == false)) {
      if(_DEBUG_) { Serial.print("Died due to oscillation at generation "); Serial.println(generation); }
      end_before_next_time = true;
    }
    
    // Death due to solo glider
    // Gliders are relatively boring
    if(( next_equals_glider() == 1 ) and (end_before_next_time == false)) {
      if(_DEBUG_) { Serial.print("Died due to lonely glider at generation "); Serial.println(generation); }
      end_before_next_time = true;
    }

    // Death due to running too long - 1800 frames is about 15 minutes. (Probably still too long, I suspect the average methuselah is a glider.)
    // Congratulations, multi-element loop!  Time to die!
    if((generation >= 1800) and (end_before_next_time == false)) {
      if(_DEBUG_) { Serial.print("Died due to methuselah's syndrome at generation "); Serial.println(generation); }
      end_before_next_time = true;
    }
    
    // ~ 500msec per generation.
    // Otherwise, fade to the next generation
    fade_to_next_frame(50);

    // this is serial-sensitive - more time spent on the serial line increases the delay.
    unsigned int delaytime = abs(505 - (abs(millis()) - curtime)); 
    if(delaytime >= 500) { delaytime = 90; }
    delay(delaytime);

    frame_number++;
    generation++;
    temp_generation++;

    if(frame_number >= 20 ) {
      frame_number = 0;
    }
  }
}


void generate_next_generation(void){  //looks at current generation, writes to next generation array
  char x,y, neighbors;
  for ( y=0; y<ROWS; y++ ) {
    for ( x=0; x<COLS; x++ ) {
      //count the number of current neighbors - currently planar.
      neighbors = 0;
      if( get_led_xy((x-1),(y-1)) > 0 ) { neighbors++; } //NW
      if( get_led_xy(( x ),(y-1)) > 0 ) { neighbors++; } //N
      if( get_led_xy((x+1),(y-1)) > 0 ) { neighbors++; } //NE
      if( get_led_xy((x-1),( y )) > 0 ) { neighbors++; } //W
      if( get_led_xy((x+1),( y )) > 0 ) { neighbors++; } //E
      if( get_led_xy((x-1),(y+1)) > 0 ) { neighbors++; } //SW
      if( get_led_xy(( x ),(y+1)) > 0 ) { neighbors++; } //S
      if( get_led_xy((x+1),(y+1)) > 0 ) { neighbors++; } //SE

      if( world[x][y][0] > 0 ){
        //current cell is alive
        if( neighbors < 2 ){
          //Any live cell with fewer than two live neighbours dies, as if caused by under-population
          world[x][y][1] = 0;
        }
        if( (neighbors == 2) || (neighbors == 3) ){
          //Any live cell with two or three live neighbours lives on to the next generation
	  if(AGING) {
	    if(get_led_xy(x,y) > 3) { world[x][y][1] = (get_led_xy(x,y)-1); }
	    else { world[x][y][1] = get_led_xy(x,y); }
	  }
	  else {
	    world[x][y][1] = 7;
	  }
        }
        if( neighbors > 3 ){
          //Any live cell with more than three live neighbours dies, as if by overcyding
          world[x][y][1] = 0;
        }
      }
      else {
        //current cell is dead
        if( neighbors == 3 ){
          // Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction
          world[x][y][1] = 7;
        }
        else {
          //stay dead for next generation
          world[x][y][1] = 0;
        }
      }
    }
  }
}

char get_led_xy (char col, char row) {
  if(TOROID == 1) {
    if(col < 0)      { col = COLS-1; }
    if(col > COLS-1) { col = 0; }
    if(row < 0)      { row = ROWS-1; }
    if(row > ROWS-1) { row = 0; }
  }
  else {
    if(col < 0 | col > COLS-1) { return 0; }
    if(row < 0 | row > ROWS-1) { return 0;  }
  }
  return world[col][row][0];
}

void DisplayTime(unsigned long int runtime) {
  LedSign::Clear();
  
  int x;
  char text[2];
  // hours on the top
  itoa(hours,text,10);
  if(hours < 10) { // right justify 1-9
    x = Font_Draw(text[0],5,0,7);
  }    
  else if(hours >= 10 && hours < 20) {
    x = Font_Draw(text[0],1,0,7);
    Font_Draw(text[1],x+1,0,7);
  }
  else {
    x = Font_Draw(text[0],0,0,7);
    Font_Draw(text[1],x,0,7);
  }
  
  // minutes on the bottom
  itoa(minutes,text,10);
  if(minutes < 10) {
    x = Font_Draw(48, 0, 6, 7);
    Font_Draw(text[0], x, 6, 7);
  }
  else if (minutes >=10 && minutes <=19) {
    x = Font_Draw(text[0], 1, 6, 7);
    Font_Draw(text[1], x+1, 6, 7);
  }
  else {
    x = Font_Draw(text[0],0,6,7);
    Font_Draw(text[1],x,6,7);
  }
  delay(runtime);
}

void setTime() {
  RTC.switchTo24h();
  RTC.stop();
  // attempt to not cause the clock to lose as much time in the setting mode
  //  RTC.setSeconds(0);
  RTC.setMinutes(minutes);
  RTC.setHours(hours);
  // dummy values, since they are not displayed anyway;
  RTC.setDayOfWeek(1);
  RTC.setDate(1);
  RTC.setMonth(1);
  RTC.setYear(9);
  RTC.setClock(); // important!  Without this, all of the prior settings are lost by the next getter
  RTC.start();
}

void updateTimeBuffer() {
  RTC.readClock();
  minutes = RTC.getMinutes();
  hours   = RTC.getHours();

  // build the string containing formatted time;
  sprintf(timeBuffer, "%2d:%02d", hours, minutes);
}

void resetDisplay() {
  for(uint8_t y = 0; y <=ROWS; y++) {
    for(uint8_t x = 0; x <=COLS; x++) {
      world[x][y][0] = 0;
      world[x][y][1] = 0;
    }
  }
}

void processSetButton() {
  if((hours > 23) or (minutes > 59)) {
    updateTimeBuffer();
  }

  // debouncing;
  if (abs(millis() - lastButtonTime) < BOUNCE_TIME_BUTTON)
    return;

  lastButtonTime = millis();

  isSettingTime    = true;

  if(isSettingHours == true) {
    isSettingHours = false;
    isSettingMinutes = true;
    isSettingBrightness = false; 
  }
  else if(isSettingMinutes == true) {
    isSettingHours = false;
    isSettingMinutes = false;
    isSettingBrightness = true; 
  }
  else if(isSettingBrightness == true) {
    isSettingHours = true;
    isSettingMinutes = false;
    isSettingBrightness = false; 
  }

  if(_DEBUG_) {
    Serial.print("h: "); Serial.println(isSettingHours, DEC);
    Serial.print("m: "); Serial.println(isSettingMinutes, DEC);
    Serial.print("b: "); Serial.println(isSettingBrightness, DEC);
  }

  // this was breaking it for reasons I can't quite pin down.
  //  resetDisplay();
  
  int x;
  char text[2];
  if(isSettingHours) {
    LedSign::Clear();
    itoa(hours,text,10);
    x = Font_Draw(text[0],0,0,7);
    Font_Draw(text[1],x,0,7);
    delay(10);
  }
  else if(isSettingMinutes) {
    LedSign::Clear();
    itoa(minutes,text,10);
    x = Font_Draw(text[0],0,6,7);
    Font_Draw(text[1],x,6,7);
    delay(10);
  }
  else if(isSettingBrightness) {
    LedSign::Clear(7);
    uint8_t disp_brightness = (max_brightness-30);
    itoa(disp_brightness,text,10);
    x = Font_Draw(text[0],0,3,0);
    Font_Draw(text[1],x,3,0);
    delay(10);
  }
}

//-----------------------------------------------------------
void processIncButton() {
  if((hours > 23) or (minutes > 59)) {
    updateTimeBuffer();
  }

  // debouncing;
  if (abs(millis() - lastButtonTime) < BOUNCE_TIME_BUTTON)
    return;

  lastButtonTime = millis();

  if (isSettingHours) {
    hours++;
    if (hours > 23) hours = 0;
  }
  else if(isSettingMinutes) {
    minutes++;
    if (minutes > 59) minutes = 0;
  }
  else if(isSettingBrightness) {
    max_brightness++;
    if(max_brightness > 49)  max_brightness = 31; 
  }

  int x;
  char text[2];
  if(isSettingHours) {
    LedSign::Clear();
    itoa(hours,text,10);
    x = Font_Draw(text[0],0,0,7);
    Font_Draw(text[1],x,0,7);
    delay(10);
  }
  else if(isSettingMinutes) {
    LedSign::Clear();
    itoa(minutes,text,10);
    x = Font_Draw(text[0],0,6,7);
    Font_Draw(text[1],x,6,7);
    delay(10);
  }
  else if(isSettingBrightness) {
    LedSign::Clear(7);
    uint8_t disp_brightness = (max_brightness-30);
    itoa(disp_brightness,text,10);
    x = Font_Draw(text[0],0,3,0);
    Font_Draw(text[1],x,3,0);
    LedSign::SetBrightness(max_brightness);
    delay(10);
  }
}

void Rain(unsigned long now, unsigned long runtime) {
  // Higher numbers make for heavier rain.
  uint8_t density = random(20,80);
  // Higher numbers make for slower rain
  uint8_t rainspeed = random(2,20);
  if(_DEBUG_) { Serial.println("-----"); Serial.println("Rain starting: "); Serial.print(density, DEC); Serial.print(" "); Serial.println(rainspeed, DEC); }
  uint8_t stopraining = 0;
  while(1) {
    if(millis() > (now+runtime)) { // get out of rain eventually.
      stopraining++;
    }
    // move everything down one row
    for(uint8_t y = 0; y < ROWS; y++) {
      for(uint8_t x = 0; x < COLS; x++) {
	if(world[x][y][0] > 0) { // if there's a value in the current frame, copy it to the next frame, 1 row down
	  world[x][y+1][1] = world[x][y][0]; 
	}
	else { // otherwise blank the LED in the next frame.
	  world[x][y+1][1] = 0;
	}
      }
    }
    // fill in the now vacant top row with random lights
    for(uint8_t x = 0; x < COLS; x++) {
      if(stopraining < ROWS+1) {
	if(random(100) > density) { 
	  world[x][0][1] = random(1,7);
	}
	else {
	  world[x][0][1] = 0;
	}
      }
      else if(stopraining >= ROWS*COLS) {
	return;
      }
      else {
	world[x][0][1] = 0;
      }
    }
    // draw the changes - after this world[0] will be identical to world[1], so keep that in mind.
    fade_to_next_frame(rainspeed);
  }
}

void Breathe(unsigned long now, unsigned long runtime) {
  while(1) {
    if(millis() > (now+runtime)) {
      return;
    }
    for(uint8_t y=0; y < ROWS; y++) { for(uint8_t x=0; x < COLS; x++) { world[x][y][1] = 7; } }
    if(millis() > (now+runtime)) { return; }
    
    fade_to_next_frame(30);
    delay(300); 
    if(millis() > (now+runtime)) { return;  }
    
    for(uint8_t y=0; y < ROWS; y++) { for(uint8_t x=0; x < COLS; x++) { world[x][y][1] = 0; } }
    
    fade_to_next_frame(30);
    if(millis() > (now+runtime)) { return; }
    delay(300); 
  }
}

// Theory of operation: Bring world[x][y][0] towards world[x][y][1] and draw it.  When nothing changes, break out of the loop.
void fade_to_next_frame(uint8_t speed) {
  char x,y, changes;

  while(1) {
    changes = 0;
    for(y = 0; y < ROWS; y++) {
      for(x = 0; x < COLS; x++) {
	if( world[x][y][0] < world[x][y][1] ) {
	  world[x][y][0]++;
	  changes++;
	}
	if( world[x][y][0] > world[x][y][1] ) {
	  world[x][y][0]--;
	  changes++;
	}
      }
    }
    draw_frame();

    // give those changes a second to apply - LedSign's interrupt driven nature
    // means otherwise the whole update could happen between polling intervals
    // and it would just jump from frame to the next frame.
    delay(speed); 
    if( changes == 0 ) {
      break;
    }
  }
}

// Draws the current data in world[0].
void draw_frame (void) {
  char x, y;
  for(y=0; y < ROWS; y++) {
    for(x=0; x < COLS; x++) {
      LedSign::Set(x,y,world[x][y][0]);
    }
  }
}

/** Writes an array to the display
 * @param str is the array to display
 * @param speed is the inter-frame speed
 */
//void Banner ( String str, int speed ) { // this works, hogs memory.
void Banner ( char* str, int speed, int y) {
  // length is length(array)
  // width is the width of the array in pixels.
  // these can be unsigned - int8 might be too small
  uint8_t width=0, length=0;
  
  if(_DEBUG_) { Serial.print("Banner text: "); Serial.println(str); }
  
  // figure out pixel width of str
  while(str[length]!='\0') { //read to the end of the string
    int charwidth=0;
    length++;
    // get the character's width by calling Font_Draw in the "Don't draw this, just tell me how wide it is" mode.
    // It would probably be faster to have a lookup table.
    charwidth = Font_Draw(str[length],0,0,0);
    // adds the width of ths current character to the pixel width.
    width=width+charwidth; 
  }

  // these need to be signed, otherwise I can't draw off the left side of the screen.
  int8_t x=0, x2=0; // x position buckets
  
  // j is the virtual display width from the actual rightmost column to a 
  // virtual column off the left hand side.  Decrements, so the scroll goes
  // to the left.
  for(int8_t j=5; j>=-(width+5); j--) {  // FIXME: this might need fixing for wider arrays 
    // x starts out at j (which is always moving to the left, remember)
    x=j; 
    // clear the sign
    LedSign::Clear(); 
    // walk through the array, drawing letters where they belong.
    for(int i=0; i<length; i++) { 
      x2 = Font_Draw(str[i],x,y,7);
      // sets the new xpos based off of the old xpos + the width of the 
      // current character.
      x+=x2;
    }
    delay(speed);
  }
}


/** Draws a figure (0-Z). You should call it with set=1, 
 * wait a little them call it again with set=0
 * @param figure is the figure [0-9]
 * @param x,y coordinates, 
 * @param set is 1 or 0 to draw or clear it
 */
uint8_t Font_Draw(unsigned char letter,int x,int y,int set) {
  uint16_t maxx=0;
  
  uint8_t charCol;
  uint8_t charRow;

  prog_uchar* character;
  if (letter==' ') return 2+2; // if I'm sent a space, just tell the called that the end column is 4 spaces away.
  if (letter<fontMin || letter>fontMax) { // return a blank if the sender tries to call something outside of my font table
    return 0;
  }
  character = font[letter-fontMin];

  int i=0;

  charCol = pgm_read_byte_near(character);
  charRow = pgm_read_byte_near(character + 1);

  while (charRow != 9) { // the terminal 9 ends the font element.
    if (charCol>maxx) maxx=charCol; // increment the maximum column count
    if ( // if the resulting pixel would be onscreen, send it to LedSign::Set.
	charCol + x < COLS && 
	charCol + x >= 0 && 
	charRow + y < ROWS && 
	charRow + y >= 0
	) {
      LedSign::Set(charCol + x, charRow + y, set);
    } 
    i+=2; // only get the even elements of the array.

    charCol = pgm_read_byte_near(character + i);
    charRow = pgm_read_byte_near(character + i + 1);
  }
  return maxx+2; // return the rightmost column + 2 for spacing.
}

// requests the DS18B20 start to perform a temperature conversion.
void RequestDS18B20Temp(void) {

  if ( OneWire::crc8( ds_addr, 7) != ds_addr[7]) {
    return;
  }

  ds.reset();
  ds.select(ds_addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end
}

// gets the results of the DS18B20's temperature conversion - needs to happen at
// least 750ms after the request.  Returns temperature in c, with the fractional
// part glommed on, so 25.32C is 2532.
float GetDS18B20Temp(void) {
  int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
  byte data[12];
  float temp;
  
  ds.reset();
  ds.select(ds_addr);
  ds.write(0xBE);         // Read Scratchpad
  
  for ( byte i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
  
  LowByte = data[0]; HighByte = data[1];
  TReading = (HighByte << 8) + LowByte;
  SignBit = TReading & 0x8000;  // test most sig bit
  
  if (SignBit) { // negative
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }

  Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25

  temp = ( (float) Tc_100 / 100 );

  return(temp);
}



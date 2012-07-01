/*
  USE_GITHUB_USERNAME=pfriedel
*/

#include <EEPROM.h>        // stock header
#include <avr/pgmspace.h>  //AVR library for writing to ROM
#include "Charliplexing.h"
#include "RealTimeClockDS1307.h"
#include "tinyfont.h" 
#include <stdio.h>

#define COLS 10 // usually x
#define ROWS 11 // usually y

// set to true to enable serial monitor - warning: uses ~2k of progmem and just
// shy of 256 bytes of ram.
#define _DEBUG_ true

uint8_t max_brightness=13;

byte world[COLS][ROWS][2]; // Create a double buffered world.

//--------------------------------------------------------------------------------
void setup() {

  if(_DEBUG_) { Serial.begin(115200); }

  // Initialize the screen  
  LedSign::Init(GRAYSCALE);
  LedSign::SetBrightness(31);

}

void loop() {

//  for(int x = 4; x<=7; x++) { 
//    Serial.print(x-3); Serial.print(" ");
//    Serial.print(x-2); Serial.print(" ");
//    Serial.print(x-1); Serial.print(" ");
//    Serial.print(x); Serial.print(" ");
//    Serial.print(x+1); Serial.print(" ");
//    Serial.print(x+2); Serial.print(" ");
//    Serial.print(x+3); Serial.print(" ");
//    Serial.println();
//
//    LedSign::drawLine(0, 0, 9, 9,x);
//    LedSign::drawLine(0, 1, 9,10,x+1);
//    LedSign::drawLine(0, 2, 8,10,x+2);
//    LedSign::drawLine(0, 3, 7,10,x+3);
//    LedSign::drawLine(0, 4, 6,10,x+2);
//    LedSign::drawLine(0, 5, 5,10,x+1);
//    LedSign::drawLine(0, 6, 4,10,x);
//    LedSign::drawLine(0, 7, 3,10,x-1);
//    LedSign::drawLine(0, 8, 2,10,x-2);
//    LedSign::drawLine(0, 9, 1,10,x-3);
//    LedSign::Set(0,10,x-2);
//    LedSign::drawLine(1,0,9,8,x-1);
//    LedSign::drawLine(2,0,9,7,x-2);
//    LedSign::drawLine(3,0,9,6,x-3);
//    LedSign::drawLine(4,0,9,5,x-2);
//    LedSign::drawLine(5,0,9,4,x-1);
//    LedSign::drawLine(6,0,9,3,x);
//    LedSign::drawLine(7,0,9,2,x+1);
//    LedSign::drawLine(8,0,9,1,x+2);
//    LedSign::Set(9,0,x+3);
//    delay(50);
//  }
//  for(int x = 7; x>=4; x--) {
//    Serial.print(x-3); Serial.print(" ");
//    Serial.print(x-2); Serial.print(" ");
//    Serial.print(x-1); Serial.print(" ");
//    Serial.print(x); Serial.print(" ");
//    Serial.print(x+1); Serial.print(" ");
//    Serial.print(x+2); Serial.print(" ");
//    Serial.print(x+3); Serial.print(" ");
//    Serial.println();
//
//    LedSign::drawLine(0, 0, 9, 9,x);
//    LedSign::drawLine(0, 1, 9,10,x+1);
//    LedSign::drawLine(0, 2, 8,10,x+2);
//    LedSign::drawLine(0, 3, 7,10,x+3);
//    LedSign::drawLine(0, 4, 6,10,x+2);
//    LedSign::drawLine(0, 5, 5,10,x+1);
//    LedSign::drawLine(0, 6, 4,10,x);
//    LedSign::drawLine(0, 7, 3,10,x-1);
//    LedSign::drawLine(0, 8, 2,10,x-2);
//    LedSign::drawLine(0, 9, 1,10,x-3);
//    LedSign::Set(0,10,x-2);
//    LedSign::drawLine(1,0,9,8,x-1);
//    LedSign::drawLine(2,0,9,7,x-2);
//    LedSign::drawLine(3,0,9,6,x-3);
//    LedSign::drawLine(4,0,9,5,x-2);
//    LedSign::drawLine(5,0,9,4,x-1);
//    LedSign::drawLine(6,0,9,3,x);
//    LedSign::drawLine(7,0,9,2,x+1);
//    LedSign::drawLine(8,0,9,1,x+2);
//    LedSign::Set(9,0,x+3);
//    delay(50);
//  }

  int counter = 6;
  int direction = 1;
  while(1) {
    int internal = counter%8;

    Serial.println(counter);
    Serial.println(internal);
    LedSign::Horizontal( 0,(internal+0));
    LedSign::Horizontal( 1,(internal+1));
    LedSign::Horizontal( 2,(internal+2));
    LedSign::Horizontal( 3,(internal+3));
    LedSign::Horizontal( 4,(internal+4));
    LedSign::Horizontal( 5,(internal+5));
    LedSign::Horizontal( 6,(internal+4));
    LedSign::Horizontal( 7,(internal+3));
    LedSign::Horizontal( 8,(internal+2));
    LedSign::Horizontal( 9,(internal+1));
    LedSign::Horizontal(10,(internal+0));
    delay(100);

    //    if((counter == 7) || (counter == 0)) { direction = !direction; }

    if(direction == 1) {
      counter++;
    }
    else {
      counter--;
    }
  }
}

//--------------------------------------------------------------------------------
// functions


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


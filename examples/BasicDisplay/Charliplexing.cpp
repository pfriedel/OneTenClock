/*
  Charliplexing.cpp - Using timer2 with 1ms resolution
  
  Alex Wenger <a.wenger@gmx.de> http://arduinobuch.wordpress.com/
  Matt Mets <mahto@cibomahto.com> http://cibomahto.com/
  
  Timer init code from MsTimer2 - Javier Valencia <javiervalencia80@gmail.com>
  Misc functions from Benjamin Sonnatg <benjamin@sonntag.fr>
  
  History:
    2009-12-30 - V0.0 wrote the first version at 26C3/Berlin
    2010-01-01 - V0.1 adding misc utility functions 
      (Clear, Vertical,  Horizontal) comment are Doxygen complaints now
    2010-05-27 - V0.2 add double-buffer mode
    2010-08-18 - V0.9 Merge brightness and grayscale
    2012-05-04 - Modified for various size charlieplexes.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <inttypes.h>
#include <math.h>
#include <avr/interrupt.h>
#include "Charliplexing.h"

#define swap(a, b) { uint16_t t = a; a = b; b = t; }

volatile unsigned int LedSign::tcnt2;

struct videoPage {
// WAS: 48 - seems to be needed for double buffering, but since I'm only using the single page, I only need half the allocation.
  uint8_t pixels[SHADES][24];  // TODO: is 48 right?
}; 

/* -----------------------------------------------------------------  */
/** Table for the LED multiplexing cycles
 * Each frame is made of 24 bytes (for the 24 display cycles)
 * There are SHADES frames per buffer in grayscale mode (one for each brigtness)
 * and twice that many to support double-buffered grayscale.
 */
// WAS: [2] - [1] still seems to work, halves the memory utilization from 768 to 384 to 192 bytes.  That's quite reasonable.
videoPage leds[1];

/// Determines whether the display is in single or double buffer mode
uint8_t displayMode = SINGLE_BUFFER;

/// Flag indicating that the display page should be flipped as soon as the
/// current frame is displayed
volatile boolean videoFlipPage = false;

/// Pointer to the buffer that is currently being displayed
videoPage* displayBuffer;

/// Pointer to the buffer that should currently be drawn to
videoPage* workBuffer;

/// Flag indicating that the timer buffer should be flipped as soon as the
/// current frame is displayed
volatile boolean videoFlipTimer = false;


// Timer counts to display each page for, plus off time
typedef struct timerInfo {
    uint8_t counts[SHADES];
    uint8_t prescaler[SHADES];
};

// Double buffer the timing information, of course.
timerInfo* frontTimer;
timerInfo* backTimer;

timerInfo* tempTimer;

timerInfo timer[2];

// Record a slow and fast prescaler for later use
typedef struct prescalerInfo {
    uint8_t relativeSpeed;
    uint8_t TCCR2;
};

// TODO: Generate these based on processor type and clock speed
prescalerInfo slowPrescaler = {1, 0x03};
//prescalerInfo fastPrescaler = {32, 0x01};
prescalerInfo fastPrescaler = {4, 0x02};

static bool initialized = false;

/// Uncomment to set analog pin 5 high during interrupts, so that an
/// oscilloscope can be used to measure the processor time taken by it
//#define MEASURE_ISR_TIME
#ifdef MEASURE_ISR_TIME
uint8_t statusPIN = 19;
#endif

typedef struct LEDPosition {
    uint8_t high;
    uint8_t low;
};


/* -----------------------------------------------------------------  */
/** Table for LED Position in leds[] ram table
 */
// CHANGEME:

int DisplayRows = 11;
int DisplayCols = 10;

// 5mm layout
const LEDPosition ledMap[110] = {
  {12, 2}, {12, 3}, {12, 4}, {12, 5}, {12, 6}, {12, 7}, {12, 8}, {12, 9}, {12, 10}, {12, 11},
  {11, 2}, {11, 3}, {11, 4}, {11, 5}, {11, 6}, {11, 7}, {11, 8}, {11, 9}, {11, 10}, {11, 12},
  {10, 2}, {10, 3}, {10, 4}, {10, 5}, {10, 6}, {10, 7}, {10, 8}, {10, 9}, {10, 11}, {10, 12},
  {9, 2}, {9, 3}, {9, 4}, {9, 5}, {9, 6}, {9, 7}, {9, 8}, {9, 10}, {9, 11}, {9, 12},
  {8, 2}, {8, 3}, {8, 4}, {8, 5}, {8, 6}, {8, 7}, {8, 9}, {8, 10}, {8, 11}, {8, 12},
  {7, 2}, {7, 3}, {7, 4}, {7, 5}, {7, 6}, {7, 8}, {7, 9}, {7, 10}, {7, 11}, {7, 12},
  {6, 2}, {6, 3}, {6, 4}, {6, 5}, {6, 7}, {6, 8}, {6, 9}, {6, 10}, {6, 11}, {6, 12},
  {5, 2}, {5, 3}, {5, 4}, {5, 6}, {5, 7}, {5, 8}, {5, 9}, {5, 10}, {5, 11}, {5, 12},
  {4, 2}, {4, 3}, {4, 5}, {4, 6}, {4, 7}, {4, 8}, {4, 9}, {4, 10}, {4, 11}, {4, 12},
  {3, 2}, {3, 4}, {3, 5}, {3, 6}, {3, 7}, {3, 8}, {3, 9}, {3, 10}, {3, 11}, {3, 12},
  {2, 3}, {2, 4}, {2, 5}, {2, 6}, {2, 7}, {2, 8}, {2, 9}, {2, 10}, {2, 11}, {2, 12},
};

// 3mm layout
//const LEDPosition ledMap[110] = {
//  {12,2}, {11, 2}, {10, 2}, { 9, 2}, { 8, 2}, { 7, 2}, { 6, 2}, { 5, 2}, { 4, 2}, { 3, 2},
//  {12,3}, {11, 3}, {10, 3}, { 9, 3}, { 8, 3}, { 7, 3}, { 6, 3}, { 5, 3}, { 4, 3}, { 2, 3},
//  {12,4}, {11, 4}, {10, 4}, { 9, 4}, { 8, 4}, { 7, 4}, { 6, 4}, { 5, 4}, { 3, 4}, { 2, 4},
//  {12,5}, {11, 5}, {10, 5}, { 9, 5}, { 8, 5}, { 7, 5}, { 6, 5}, { 4, 5}, { 3, 5}, { 2, 5},
//  {12,6}, {11, 6}, {10, 6}, { 9, 6}, { 8, 6}, { 7, 6}, { 5, 6}, { 4, 6}, { 3, 6}, { 2, 6},
//  {12,7}, {11, 7}, {10, 7}, { 9, 7}, { 8, 7}, { 6, 7}, { 5, 7}, { 4, 7}, { 3, 7}, { 2, 7},
//  {12,8}, {11, 8}, {10, 8}, { 9, 8}, { 7, 8}, { 6, 8}, { 5, 8}, { 4, 8}, { 3, 8}, { 2, 8},
//  {12,9}, {11, 9}, {10, 9}, { 8, 9}, { 7, 9}, { 6, 9}, { 5, 9}, { 4, 9}, { 3, 9}, { 2, 9},
//  {12,10}, {11,10}, { 9,10}, { 8,10}, { 7,10}, { 6,10}, { 5,10}, { 4,10}, { 3,10}, { 2,10},
//  {12,11}, {10,11}, { 9,11}, { 8,11}, { 7,11}, { 6,11}, { 5,11}, { 4,11}, { 3,11}, { 2,11},
//  {11,12}, {10,12}, { 9,12}, { 8,12}, { 7,12}, { 6,12}, { 5,12}, { 4,12}, { 3,12}, { 2,12},
//};

/* -----------------------------------------------------------------  */
/** Constructor : Initialize the interrupt code. 
 * should be called in setup();
 */
void LedSign::Init(uint8_t mode)
{
#ifdef MEASURE_ISR_TIME
    pinMode(statusPIN, OUTPUT);
    digitalWrite(statusPIN, LOW);
#endif

	float prescaler = 0.0;
	
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega48__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega328P__) || (__AVR_ATmega1280__)
	TIMSK2 &= ~(1<<TOIE2);
	TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
	TCCR2B &= ~(1<<WGM22);
	ASSR &= ~(1<<AS2);
	TIMSK2 &= ~(1<<OCIE2A);
	
	if ((F_CPU >= 1000000UL) && (F_CPU <= 16000000UL)) {	// prescaler set to 64
		TCCR2B |= ((1<<CS21) | (1<<CS20));
		TCCR2B &= ~(1<<CS22);
		prescaler = 32.0;
	} else if (F_CPU < 1000000UL) {	// prescaler set to 8
		TCCR2B |= (1<<CS21);
		TCCR2B &= ~((1<<CS22) | (1<<CS20));
		prescaler = 8.0;
	} else { // F_CPU > 16Mhz, prescaler set to 128
		TCCR2B |= (1<<CS22);
		TCCR2B &= ~((1<<CS21) | (1<<CS20));
		prescaler = 64.0;
	}
#elif defined (__AVR_ATmega8__)
	TIMSK &= ~(1<<TOIE2);
	TCCR2 &= ~((1<<WGM21) | (1<<WGM20));
	TIMSK &= ~(1<<OCIE2);
	ASSR &= ~(1<<AS2);
	
	if ((F_CPU >= 1000000UL) && (F_CPU <= 16000000UL)) {	// prescaler set to 64
		TCCR2 |= (1<<CS22);
		TCCR2 &= ~((1<<CS21) | (1<<CS20));
		prescaler = 64.0;
	} else if (F_CPU < 1000000UL) {	// prescaler set to 8
		TCCR2 |= (1<<CS21);
		TCCR2 &= ~((1<<CS22) | (1<<CS20));
		prescaler = 8.0;
	} else { // F_CPU > 16Mhz, prescaler set to 128
		TCCR2 |= ((1<<CS22) && (1<<CS20));
		TCCR2 &= ~(1<<CS21);
		prescaler = 128.0;
	}
#elif defined (__AVR_ATmega128__)
	TIMSK &= ~(1<<TOIE2);
	TCCR2 &= ~((1<<WGM21) | (1<<WGM20));
	TIMSK &= ~(1<<OCIE2);
	
	if ((F_CPU >= 1000000UL) && (F_CPU <= 16000000UL)) {	// prescaler set to 64
		TCCR2 |= ((1<<CS21) | (1<<CS20));
		TCCR2 &= ~(1<<CS22);
		prescaler = 64.0;
	} else if (F_CPU < 1000000UL) {	// prescaler set to 8
		TCCR2 |= (1<<CS21);
		TCCR2 &= ~((1<<CS22) | (1<<CS20));
		prescaler = 8.0;
	} else { // F_CPU > 16Mhz, prescaler set to 256
		TCCR2 |= (1<<CS22);
		TCCR2 &= ~((1<<CS21) | (1<<CS20));
		prescaler = 256.0;
	}
#endif

	tcnt2 = 256 - (int)((float)F_CPU * 0.0005 / prescaler);

    // Record whether we are in single or double buffer mode
    displayMode = mode;
    videoFlipPage = false;

    // Point the display buffer to the first physical buffer
    displayBuffer = &leds[0];

    // If we are in single buffered mode, point the work buffer
    // at the same physical buffer as the display buffer.  Otherwise,
    // point it at the second physical buffer.
    if( displayMode & DOUBLE_BUFFER ) {
        workBuffer = &leds[1];
    }
    else {
        workBuffer = displayBuffer;
    }

    // Set up the timer buffering
    frontTimer = &timer[0];
    backTimer = &timer[1];

    videoFlipTimer = false;
    SetBrightness(127);
	
    // Clear the buffer and display it
    LedSign::Clear(0);
    LedSign::Flip(false);

    // Then start the display
	TCNT2 = tcnt2;
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega48__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega328P__) || (__AVR_ATmega1280__)
	TIMSK2 |= (1<<TOIE2);
#elif defined (__AVR_ATmega128__) || defined (__AVR_ATmega8__)
	TIMSK |= (1<<TOIE2);
#endif

    // If we are in double-buffer mode, wait until the display flips before we
    // return
    if (displayMode & DOUBLE_BUFFER)
    {
        while (videoFlipPage) {
            delay(1);
        }
    }

    initialized = true;
}


/* -----------------------------------------------------------------  */
/** Signal that the front and back buffers should be flipped
 * @param blocking if true : wait for flip before returning, if false :
 *                 return immediately.
 */
void LedSign::Flip(bool blocking)
{
    if (displayMode & DOUBLE_BUFFER)
    {
        // Just set the flip flag, the buffer will flip between redraws
        videoFlipPage = true;

        // If we are blocking, sit here until the page flips.
        while (blocking && videoFlipPage) {
            delay(1);
        }
    }
}


/* -----------------------------------------------------------------  */
/** Clear the screen completely
 * @param set if 1 : make all led ON, if not set or 0 : make all led OFF
 */
void LedSign::Clear(int set) {
    for(int x=0;x<DisplayCols;x++)  
        for(int y=0;y<DisplayRows;y++) 
            Set(x,y,set);
}


/* -----------------------------------------------------------------  */
/** Clear an horizontal line completely
 * @param y is the y coordinate of the line to clear/light [0-8]
 * @param set if 1 : make all led ON, if not set or 0 : make all led OFF
 */
void LedSign::Horizontal(int y, int set) {
    for(int x=0;x<DisplayCols;x++)  
        Set(x,y,set);
}


/* -----------------------------------------------------------------  */
/** Clear a vertical line completely
 * @param x is the x coordinate of the line to clear/light [0-13]
 * @param set if 1 : make all led ON, if not set or 0 : make all led OFF
 */
void LedSign::Vertical(int x, int set) {
    for(int y=0;y<DisplayRows;y++)  
        Set(x,y,set);
}


/* -----------------------------------------------------------------  */
/** Set : switch on and off the leds. All the position #for char in frameString:
 * calculations are done here, so we don't need to do in the
 * interrupt code
 */
void LedSign::Set(uint8_t x, uint8_t y, uint8_t c)
{
  if((x > DisplayCols) || (y > DisplayRows)) { return; }
  // CHANGEME: This used to reference the number of columns as an integer, but DisplayCols is known, so calculate against that.
    uint8_t pin_high = ledMap[x+y*DisplayCols].high;
    uint8_t pin_low  = ledMap[x+y*DisplayCols].low;
    if(x==0 && y==0) { pin_high = 12; pin_low = 2; } // HACK HACK HACK - without this, ledMap[0].high = 0;
    // pin_low is directly the address in the led array (minus 2 because the 
    // first two bytes are used for RS232 communication), but
    // as it is a two byte array we need to check pin_high also.
    // If pin_high is bigger than 8 address has to be increased by one

    uint8_t bufferNum = (pin_low-2)*2 + (pin_high / 8) + ((pin_high > 7)?24:0);
    uint8_t work = _BV(pin_high & 0x07);

    // If we aren't in grayscale mode, just map any pin brightness to max
    if (c > 0 && !(displayMode & GRAYSCALE)) {
        c = SHADES-1;
    }

    for (int i = 0; i < SHADES-1; i++) {
        if( c > i ) {
            workBuffer->pixels[i][bufferNum] |= work;   // ON
        }
        else {
            workBuffer->pixels[i][bufferNum] &= ~work;   // OFF
        }
    }
}

// Imported functions from the adafruit ht1632 library.  Untested.
void LedSign::drawLine(int8_t x0, int8_t y0, int8_t x1, int8_t y1, uint8_t color) {
  uint16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }
  
  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }
  
  uint16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);
  
  int16_t err = dx / 2;
  int16_t ystep;
  
  if (y0 < y1) {
    ystep = 1;
  } 
  else {
    ystep = -1;
  }
  
  for (; x0<=x1; x0++) {
    if (steep) {
      Set(y0, x0, color);
    }
    else {
      Set(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

// draw a rectangle
void LedSign::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
  drawLine(x, y, x+w-1, y, color);
  drawLine(x, y+h-1, x+w-1, y+h-1, color);

  drawLine(x, y, x, y+h-1, color);
  drawLine(x+w-1, y, x+w-1, y+h-1, color);
}

// fill a rectangle
void LedSign::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
  for (uint8_t i=x; i<x+w; i++) {
    for (uint8_t j=y; j<y+h; j++) {
      Set(i, j, color);
    }
  }
}

// draw a circle outline
void LedSign::drawCircle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  Set(x0, y0+r, color);
  Set(x0, y0-r, color);
  Set(x0+r, y0, color);
  Set(x0-r, y0, color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  
    Set(x0 + x, y0 + y, color);
    Set(x0 - x, y0 + y, color);
    Set(x0 + x, y0 - y, color);
    Set(x0 - x, y0 - y, color);
   
    Set(x0 + y, y0 + x, color);
    Set(x0 - y, y0 + x, color);
    Set(x0 + y, y0 - x, color);
    Set(x0 - y, y0 - x, color);
  }
}

// fill a circle
void LedSign::fillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  drawLine(x0, y0-r, x0, y0+r+1, color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
 
    drawLine(x0+x, y0-y, x0+x, y0+y+1, color);
    drawLine(x0-x, y0-y, x0-x, y0+y+1, color);
    drawLine(x0+y, y0-x, x0+y, y0+x+1, color);
    drawLine(x0-y, y0-x, x0-y, y0+x+1, color);
  }
}

/* Set the overall brightness of the screen
 * @param brightness LED brightness, from 0 (off) to 127 (full on)
 */

void LedSign::SetBrightness(uint8_t brightness)
{
    // An exponential fit seems to approximate a (perceived) linear scale
    float brightnessPercent = ((float)brightness / 127)*((float)brightness / 127);
    uint8_t difference = 0;

    /*   ---- This needs review! Please review. -- thilo  */
    // set up page counts
    // TODO: make SHADES a function parameter. This would require some refactoring.
    int start = 15;
    int max = 255;
    float scale = 1.5;
    float delta = pow( max - start , 1.0 / scale) / (SHADES - 1);
    uint8_t pageCounts[SHADES]; 

    pageCounts[0] = max - start;
    for (uint8_t i=1; i<SHADES; i++) {
        pageCounts[i] = max - ( pow( i * delta, scale ) + start );
    }
//    Serial.end();

    if (! initialized) {
       // set front timer defaults
        for (int i = 0; i < SHADES; i++) {
            frontTimer->counts[i] = pageCounts[i];
            // TODO: Generate this dynamically
            frontTimer->prescaler[i] = slowPrescaler.TCCR2;
        }
    }

    // Wait until the previous brightness request goes through
    while( videoFlipTimer ) {
        delay(1);
    }

    // Compute on time for each of the pages
    // Use the fast timer; slow timer is only useful for < 3 shades.
    for (uint8_t i = 0; i < SHADES - 1; i++) {
        uint8_t interval = 255 - pageCounts[i];

        backTimer->counts[i] = 255 -    brightnessPercent 
                                      * interval 
                                      * fastPrescaler.relativeSpeed;
        backTimer->prescaler[i] = fastPrescaler.TCCR2;
        difference += backTimer->counts[i] - pageCounts[i];
    }

    // Compute off time
    backTimer->counts[SHADES - 1] = 255 - difference;
    backTimer->prescaler[SHADES - 1] = slowPrescaler.TCCR2;

    /*   ---- End of "This needs review! Please review." -- thilo  */

    // Have the ISR update the timer registers next run
    videoFlipTimer = true;
}


/* -----------------------------------------------------------------  */
/** The Interrupt code goes here !  
 */

ISR(TIMER2_OVF_vect) {
        DDRD  = 0x0;
	DDRB  = 0x0;
#ifdef MEASURE_ISR_TIME
    digitalWrite(statusPIN, HIGH);
#endif

    // For each cycle, we have potential SHADES pages to display.
    // Once every page has been displayed, then we move on to the next
    // cycle.

    // 24 Cycles of Matrix
    static uint8_t cycle = 0;

    // SHADES pages to display
    static uint8_t page = 0;

    TCCR2B = frontTimer->prescaler[page];
    TCNT2 = frontTimer->counts[page];

    if ( page < SHADES - 1) { 

        if (cycle < 6) {
            DDRD  = _BV(cycle+2) | displayBuffer->pixels[page][cycle*2];
            PORTD =            displayBuffer->pixels[page][cycle*2];

            DDRB  =            displayBuffer->pixels[page][cycle*2+1];
            PORTB =            displayBuffer->pixels[page][cycle*2+1];
        } else if (cycle < 12) {
            DDRD =             displayBuffer->pixels[page][cycle*2];
            PORTD =            displayBuffer->pixels[page][cycle*2];

            DDRB  = _BV(cycle-6) | displayBuffer->pixels[page][cycle*2+1];
            PORTB =            displayBuffer->pixels[page][cycle*2+1];      
        } else if (cycle < 18) {
            DDRD  = _BV(cycle+2-12) | displayBuffer->pixels[page][cycle*2];
            PORTD =            displayBuffer->pixels[page][cycle*2];

            DDRB  =            displayBuffer->pixels[page][cycle*2+1];
            PORTB =            displayBuffer->pixels[page][cycle*2+1];
        } else {
            DDRD =             displayBuffer->pixels[page][cycle*2];
            PORTD =            displayBuffer->pixels[page][cycle*2];

            DDRB  = _BV(cycle-6-12) | displayBuffer->pixels[page][cycle*2+1];
            PORTB =            displayBuffer->pixels[page][cycle*2+1];      
        }
    } 
    else {
        // Turn everything off
        DDRD  = 0x0;
        DDRB  = 0x0;
    }

    page++;

    if (page >= SHADES) {
        page = 0;
        cycle++;
    }

    if (cycle > 24) {
        cycle = 0;

        // If the page should be flipped, do it here.
        if (videoFlipPage && (displayMode & DOUBLE_BUFFER))
        {
            // TODO: is this an atomic operation?
            videoFlipPage = false;

            videoPage* temp = displayBuffer;
            displayBuffer = workBuffer;
            workBuffer = temp;
        }

        if (videoFlipTimer) {
            videoFlipTimer = false;

            tempTimer = frontTimer;
            frontTimer = backTimer;
            backTimer = tempTimer;
        }
    }

#ifdef MEASURE_ISR_TIME
    digitalWrite(statusPIN, LOW);
#endif
}

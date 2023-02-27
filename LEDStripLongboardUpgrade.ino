#include <FastLED.h>


//for the measurement of the speed
#define MAXSPEED 8.3
#define WHEELDIAMETER 0.060
#define NRMAGNETS 4.0
#define SPEEDPIN 2

//for the LEDstrips
#define DATA_PIN 0
#define NUM_LEDS_SIDE 30
#define LEDDISTANCE 30.0

#define NUM_LEDS_CIRCLE 16


CRGB leds[NUM_LEDS_SIDE];

/* code by Rolf Hut, based on code by Mark Kriegsman, 
 *  the animation shows a bar looping around a neopixel LED-strip.
 *  The speed is determined by reading an analog value from SPEEDINPIN
 *  Everything related to the "integer" bar from Marks code has been removed.
 */

// Anti-aliased light bar example
//   v1 by Mark Kriegsman <kriegsman@tr.org>, November 29, 2013
//
// This example shows the basics of using variable pixel brightness
// as a form of anti-aliasing to animate effects on a sub-pixel level, 
// which is useful for effects you want to be particularly "smooth".
//
// This animation shows two bars looping around an LED strip, one moving
// in blocky whole-pixel "integer" steps, and the other one moving 
// by smoothly anti-aliased "fractional" (1/16th pixel) steps.
// The use of "16ths" (vs, say, 10ths) is totally arbitrary, but 16ths are
// a good balance of data size, expressive range, and code size and speed.
//
// Notionally, "I" is the Integer Bar, "F" is the Fractional Bar.


int     F16pos = 0; // position of the "fraction-based bar"
int     F16delta = 1; // how many 16ths of a pixel to move the Fractional Bar
uint8_t Fhue = 20; // color for Fractional Bar

int Width  = 12; // width of each light bar, in whole pixels
uint8_t waveShape[13] = {16,8,4,2,1,1,1,1,1,2,4,8,16}; 
int n=0;

int InterframeDelay = 42; //ms
float restFactor=0.0;
float magnetDistance;

volatile int counter = 5;

unsigned long ledTime = 0;
unsigned long ledFlashTime = 42;

bool magnetStatus = LOW;

void setup() {
  delay(3000); // setup guard, to make sure capacitors are charged before turning on LEDs

  
  //add LEDstrips to library. Note that
  FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, NUM_LEDS_SIDE + NUM_LEDS_CIRCLE);
  FastLED.setBrightness(128);

  //set up interrupt for speed measurement

  pinMode(1,OUTPUT);
  digitalWrite(1,LOW);

  //the amount of distance between two magnets on the wheel, expressed in units of "16ths of a pixel"
  magnetDistance = (16.00 * 3.141 * WHEELDIAMETER) / (NRMAGNETS * 0.030);
  
  
}
//end setup()



void loop()
{
  if (digitalRead(SPEEDPIN) == HIGH) {
    if (magnetStatus == LOW){
      counter++;
      
    }
    magnetStatus = HIGH;
    digitalWrite(1,HIGH); 
  } else {
    magnetStatus = LOW;
    digitalWrite(1,LOW);
  }

  
  if ((millis() - ledTime) > ledFlashTime){
    ledTime = millis();
    if (counter > 0){
      float dx =  ((float)(counter * magnetDistance)) + restFactor;
      restFactor = dx - (float)((unsigned long)dx);
      counter = 0;
      
      // Update the "Fraction Bar" by 1/16th pixel every time
      F16pos += (unsigned long) dx;
      
      // wrap around at end
      // remember that F16pos contains position in "16ths of a pixel"
      // so the 'end of the strip' is (NUM_LEDS * 16)
      if( F16pos >= ((NUM_LEDS_SIDE+1) * 16)) {
        F16pos -= ((NUM_LEDS_SIDE+1) * 16);
        Fhue = (Fhue+16)%256;
      }
        
       
       // Draw everything:
       // clear the pixel buffer
       memset8( leds, 0, (NUM_LEDS_SIDE + NUM_LEDS_CIRCLE) * sizeof(CRGB));
       // draw the Fractional Bar, length=4px, hue=180
       
       drawFractionalBar( F16pos, Width, Fhue);
       
       FastLED.show();
    }
  }
}
//end loop()




// Draw a "Fractional Bar" of light starting at position 'pos16', which is counted in
// sixteenths of a pixel from the start of the strip.  Fractional positions are 
// rendered using 'anti-aliasing' of pixel brightness.
// The bar width is specified in whole pixels.
// Arguably, this is the interesting code.
void drawFractionalBar( int pos16, int width, uint8_t hue)
{
  int i = pos16 / 16; // convert from pos to raw pixel number
  uint8_t frac = pos16 & 0x0F; // extract the 'factional' part of the position 
  
  // brightness of the first pixel in the bar is 1.0 - (fractional part of position)
  // e.g., if the light bar starts drawing at pixel "57.9", then 
  // pixel #57 should only be lit at 10% brightness, because only 1/10th of it
  // is "in" the light bar:
  //
  //                       57.9 . . . . . . . . . . . . . . . . . 61.9
  //                        v                                      v
  //  ---+---56----+---57----+---58----+---59----+---60----+---61----+---62---->
  //     |         |        X|XXXXXXXXX|XXXXXXXXX|XXXXXXXXX|XXXXXXXX |  
  //  ---+---------+---------+---------+---------+---------+---------+--------->
  //                   10%       100%      100%      100%      90%        
  //
  // the fraction we get is in 16ths and needs to be converted to 256ths,
  // so we multiply by 16.  We subtract from 255 because we want a high
  // fraction (e.g. 0.9) to turn into a low brightness (e.g. 0.1)
  uint8_t firstpixelbrightness = 255 - (frac * 16);

  // if the bar is of integer length, the last pixel's brightness is the 
  // reverse of the first pixel's; see illustration above.
  uint8_t lastpixelbrightness  = 255 - firstpixelbrightness;
  
  // For a bar of width "N", the code has to consider "N+1" pixel positions,
  // which is why the "<= width" below instead of "< width".
  
  uint8_t bright;
  for( int n = 0; n <= width; n++) {
    if( n == 0) {
      // first pixel in the bar
      bright = firstpixelbrightness;
    } else if( n == width ) {
      // last pixel in the bar
      bright = lastpixelbrightness;
    } else {
      // middle pixels
      bright = 255;
    }
    if (i==0){
      for(int j =0; j < NUM_LEDS_CIRCLE; j++){
        leds[j] += CHSV( hue, 255, bright / waveShape[n]);
      }
    } else {
      leds[i+NUM_LEDS_CIRCLE-1] += CHSV( hue, 255, bright / waveShape[n]);
    }
    i++; 
    if( i == NUM_LEDS_SIDE + 1) i = 0; // wrap around
  }
}

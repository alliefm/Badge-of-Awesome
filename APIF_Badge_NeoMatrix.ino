// Code for Verizon Badge of Awesome: connected, API-driven LED badges built around WeMos ESP821166 module and WS2812b NeoPixel Matrix tiles
//This is terrible code and a suitably motivated infividual could adapt it by removing reliance on Adafruit IO and 
// removing the RGB struct (or updating to use the RGB struct exclusively)
//
/************************** Configuration ***********************************/
// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"

#include "RGB.h"

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

// set up the feeds
AdafruitIO_Feed *mode = io.feed("verizonbadge.badge5mode");
AdafruitIO_Feed *color = io.feed("verizonbadge.badge5color");
AdafruitIO_Feed *text = io.feed("verizonbadge.badge5text");
AdafruitIO_Feed *bright = io.feed("verizonbadge.badge5bright");
AdafruitIO_Feed *scroll = io.feed("verizonbadge.badge5scroll");

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    15

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 64

//Global variables used in this sketch
#define LED_BRIGHT 30 //Initial brightness; range is 0 to 255
String msg = "/API First";
//width of text to scroll is each letter of the text times 7 pixels each
int msgLength = 0-(msg.length()*6);
int modeState = 3;
int pass = 0;
int redd = 255;
int green = 0;
int blue = 0;
uint8_t rVal = 255;
uint8_t gVal = 0;
uint8_t bVal = 0;
uint8_t rOldVal = 0;
uint8_t gOldVal = 0;
uint8_t bOldVal = 0;
int scrollSpeed = 70;
int ledBrightness = 30;


// MATRIX DECLARATION:
// Parameter 1 = width of EACH NEOPIXEL MATRIX (not total display)
// Parameter 2 = height of each matrix
// Parameter 3 = number of matrices arranged horizontally
// Parameter 4 = number of matrices arranged vertically
// Parameter 5 = pin number (most are valid)
// Parameter 6 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the FIRST MATRIX; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs WITHIN EACH MATRIX are
//     arranged in horizontal rows or in vertical columns, respectively;
//     pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns WITHIN
//     EACH MATRIX proceed in the same order, or alternate lines reverse
//     direction; pick one.
//   NEO_TILE_TOP, NEO_TILE_BOTTOM, NEO_TILE_LEFT, NEO_TILE_RIGHT:
//     Position of the FIRST MATRIX (tile) in the OVERALL DISPLAY; pick
//     two, e.g. NEO_TILE_TOP + NEO_TILE_LEFT for the top-left corner.
//   NEO_TILE_ROWS, NEO_TILE_COLUMNS: the matrices in the OVERALL DISPLAY
//     are arranged in horizontal rows or in vertical columns, respectively;
//     pick one or the other.
//   NEO_TILE_PROGRESSIVE, NEO_TILE_ZIGZAG: the ROWS/COLUMS OF MATRICES
//     (tiles) in the OVERALL DISPLAY proceed in the same order for every
//     line, or alternate lines reverse direction; pick one.  When using
//     zig-zag order, the orientation of the matrices in alternate rows
//     will be rotated 180 degrees (this is normal -- simplifies wiring).
//   See example below for these values in action.
// Parameter 7 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 pixels)
//   NEO_GRB     Pixels are wired for GRB bitstream (v2 pixels)
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA v1 pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)

// Example with three 10x8 matrices (created using NeoPixel flex strip --
// these grids are not a ready-made product).  In this application we'd
// like to arrange the three matrices side-by-side in a wide display.
// The first matrix (tile) will be at the left, and the first pixel within
// that matrix is at the top left.  The matrices use zig-zag line ordering.
// There's only one row here, so it doesn't matter if we declare it in row
// or column order.  The matrices use 800 KHz (v2) pixels that expect GRB
// color data.
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, 1, 1, LED_PIN,
  NEO_TILE_BOTTOM   + NEO_TILE_LEFT   + NEO_TILE_COLUMNS   + NEO_TILE_PROGRESSIVE +
  NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB + NEO_KHZ800);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255) };

void setup() {
  Serial.begin(57600);
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(LED_BRIGHT);
  matrix.setTextColor(colors[0]);

    // start MQTT connection to io.adafruit.com
  io.connect();

  // set up a message handler for the count feed.
  // the handleMessage function (defined below)
  // will be called whenever a message is
  // received from adafruit io.
  mode->onMessage(handleModeMessage);
  color->onMessage(handleColorMessage);
  text->onMessage(handleTextMessage);
  bright->onMessage(handleBrightMessage);
  scroll->onMessage(handleScrollMessage);

  // wait for an MQTT connection
  // NOTE: when blending the HTTP and MQTT API, always use the mqttStatus
  // method to check on MQTT connection status specifically
  //while(io.mqttStatus() < AIO_CONNECTED) {
  //  Serial.print(".");
  //  delay(500);
  //}

  // Because Adafruit IO doesn't support the MQTT retain flag, we can use the
  // get() function to ask IO to resend the last value for this feed to just
  // this MQTT client after the io client is connected.
  mode->get();

  // we are connected
  Serial.println();
  Serial.println(io.statusText());

}
//Variables to set post Setup
int x    = matrix.width();

void loop() {
  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();

//  matrix.fillScreen(0);
  if (modeState<1) {modeState = 3;}
  if (modeState>6) {modeState = 3;}
  Serial.print("modeState: ");
  Serial.println(modeState);
  switch (modeState) {
    case 1:
      Serial.println(msg);
      scrollText(msg);
      delay(500);
      break;
    case 2:
      Serial.println("CrossFade");
      crossFade(100,5);
      delay(500);
      matrix.fillScreen(0);
      delay(500);
      break;
    case 3:
      Serial.println("Logo");
      matrix.fillScreen(0);
      matrix.show();
      drawLogo();
      delay(2000);
      break;
    case 4:
      Serial.println("Wipe");
      matrix.fillScreen(0);
      colorWipe(10);
      delay(500);
      break;
    case 5:
      Serial.println("Rainbow");
      rainbow(50);
      break;
    default:
      //modeState = 3;
      break;
  }

}

// this function is called whenever a message
// is received from Adafruit IO. i
void handleModeMessage(AdafruitIO_Data *data) {

  Serial.print("received <- ");
  Serial.println(data->value());

  if (*data->value()=='2') {
    modeState = 2;
  } else if (*data->value()=='3'){
    modeState = 3;
  } else if (*data->value()=='4') {
    modeState = 4;
  } else if (*data->value()=='5')  {
    modeState = 5;
  } else if (*data->value()=='6')  {
    modeState = 6;
  } else {
    modeState = 1;
  }

}

void handleColorMessage(AdafruitIO_Data *data) {

  Serial.print("received <- ");
  Serial.println(data->value());
  
  String hexval = data->value();
  colorConverter(hexval);
  matrix.Color(rVal,gVal,bVal);
  matrix.setTextColor(matrix.Color(rVal,gVal,bVal));
  //strip.Color(red,green,blue);
}

void handleTextMessage(AdafruitIO_Data *data) {

  Serial.print("received <- ");
  Serial.println(data->value());
  msg = data->value();
  //width of text to scroll is each letter of the text times 7 pixels each
  msgLength = 0-(msg.length()*6);

}

void handleBrightMessage(AdafruitIO_Data *data) {

  Serial.print("received <- ");
  Serial.println(data->value());
  ledBrightness = *data->value();
  if (ledBrightness>255) { ledBrightness=255;}
  if (ledBrightness<0) { ledBrightness=0;}
  matrix.setBrightness(ledBrightness);
}

void handleScrollMessage(AdafruitIO_Data *data) {

  Serial.print("received <- ");
  Serial.println(data->value());
  scrollSpeed = *data->value();

}

void colorConverter(String hexValue)
{
  long number = (long) strtol( &hexValue[1], NULL, 16);
  redd = number >> 16;
  green = number >> 8 & 0xFF;
  blue = number & 0xFF;
  rVal = number >> 16;
  gVal = number >> 8 & 0xFF;
  bVal = number & 0xFF;
  Serial.println("convert color:");
  Serial.println(redd);
  Serial.println(green);
  Serial.println(blue);
}

//scroll a message
void scrollText(String textToDisplay) {
  int x = matrix.width();

  // Account for 6 pixel wide characters plus a space
  int pixelsInText = textToDisplay.length() * 6;

  matrix.setCursor(x, 0);
  matrix.print(textToDisplay);
  matrix.show();

  while(x > (matrix.width() - pixelsInText)) {
    matrix.fillScreen(0);
    matrix.setCursor(--x, 0);
    matrix.print(textToDisplay);
    matrix.show();
    delay(scrollSpeed);
  }

  /*
  //  matrix.setCursor(x, 0);
  //  matrix.print(msg);
  //  if(--x < msgLength) {
  //    x = matrix.width();
  //    if(++pass >= 3) pass = 0;
  //    matrix.setTextColor(colors[pass]);
  //  }
  */
}
// Fill the pixels one after the other with a color
void colorWipe(uint8_t wait) {
  for(uint16_t row=0; row < 8; row++) {
    for(uint16_t column=0; column < 8; column++) {
      matrix.drawPixel(column, row, matrix.Color(rVal,gVal,bVal));
      matrix.show();
      delay(wait);
    }
  }
}

// Fade pixel (x, y) from startColor to endColor
void fadePixel(int x, int y, int steps, int wait) {
  for(int i = 0; i <= steps; i++) 
  {
     int newR = rOldVal + (rOldVal - rVal) * i / steps;
     int newG = gOldVal + (gOldVal - gVal) * i / steps;
     int newB = bOldVal + (bOldVal - bVal) * i / steps;

     matrix.drawPixel(x, y, matrix.Color(newR, newG, newB));
     matrix.show();
     delay(wait);
  }
}

// Fade pixel (x, y) from startColor to endColor
void fadePixelRGB(int x, int y, RGB startColor, RGB endColor, int steps, int wait) {
  for(int i = 0; i <= steps; i++) 
  {
     int newR = startColor.r + (endColor.r - startColor.r) * i / steps;
     int newG = startColor.g + (endColor.g - startColor.g) * i / steps;
     int newB = startColor.b + (endColor.b - startColor.b) * i / steps;

     matrix.drawPixel(x, y, matrix.Color(newR, newG, newB));
     matrix.show();
     delay(wait);
  }
}

// Crossfade entire screen from startColor to endColor
void crossFade(int steps, int wait) {
  for(int i = 0; i <= steps; i++)
  {
     int newR = rOldVal + (rOldVal - rVal) * i / steps;
     int newG = gOldVal + (gOldVal - gVal) * i / steps;
     int newB = bOldVal + (bOldVal - bVal) * i / steps;
     matrix.fillScreen(matrix.Color(newR, newG, newB));
     matrix.show();
     delay(wait);
  }
}

void drawLogo() {
  // This 8x8 array represents the LED matrix pixels. 
  // A value of 1 means we’ll fade the pixel to white
  int logo[8][8] = {  
   {0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 1},
   {0, 0, 0, 0, 0, 0, 1, 0},
   {0, 0, 0, 0, 0, 1, 0, 0},
   {1, 0, 0, 0, 1, 0, 0, 0},
   {0, 1, 0, 1, 0, 0, 0, 0},
   {0, 0, 1, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0}
  };

  for (int column = 0; column<8; column++) {
    for(int row = 0; row<9; row++) {
      if(logo[row][column] == 1) {
        fadePixelRGB(column,row,off,red,50,0);
        /*
        for(int i = 0; i <= 50; i++) 
        {
          int newR = 0 + (0 - 255) * i / 50;
          int newG = 0;
          int newB = 0;

          matrix.drawPixel(column, row, matrix.Color(newR, newG, newB));
          matrix.show();
          delay(0);
        }
        */
      }
    }
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this loop:
  for(long firstPixelHue = 0; firstPixelHue < 1*65536; firstPixelHue += 256) {
    // strip.rainbow() can take a single argument (first pixel hue) or
    // optionally a few extras: number of rainbow repetitions (default 1),
    // saturation and value (brightness) (both 0-255, similar to the
    // ColorHSV() function, default 255), and a true/false flag for whether
    // to apply gamma correction to provide 'truer' colors (default true).
    matrix.rainbow(firstPixelHue);
    // Above line is equivalent to:
    // strip.rainbow(firstPixelHue, 1, 255, 255, true);
    matrix.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

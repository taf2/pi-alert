/**
    @filename   :   EPD_5in65f.ino
    @brief      :   EPD_5in65 e-paper F display demo
    @author     :   Waveshare

    Copyright (C) Waveshare     July 8 2020

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documnetation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to  whom the Software is
   furished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

/*
 * Wiring configuration with an itsybitsy m4
 *
 * #define RST_PIN         A3
 * #define DC_PIN          A4
 * #define CS_PIN          A5
 * #define BUSY_PIN        A2
 *
 * be sure to modify this in include/epdif.h
 *
 * we'll show quotes from http://quotes.rest/qod.json?category=management
 * along with the current time
 * with the temp sensor connected we also show the temp
 **/

#include <Wire.h>
#include <SPI.h>
#include "epdif.h"
#include "imagedata.h"
#include "epd5in65f.h"

// let's see if we can use gfx library from Adafruit
#include <GxEPD2_BW.h>  // Include GxEPD2 library for black and white displays
#include <GxEPD2_3C.h>  // Include GxEPD2 library for 3 color displays
#include <Adafruit_GFX.h>  // Include Adafruit_GFX library
// Include fonts from Adafruit_GFX
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMono24pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

// seems Adafruit_GFX maybe only supports 3 colors but we have a 7 color display... we'll find out
GxEPD2_3C<GxEPD2_565c, GxEPD2_565c::HEIGHT> display(GxEPD2_565c(/*CS=10*/ CS_PIN, /*DC=*/ DC_PIN, /*RST=*/ RST_PIN, /*BUSY=*/ BUSY_PIN)); // GDEW075Z08 800x480
//#endif

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens
/*  Epd epd;
  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed");
    return;
  }
  */
  
//  Serial.print("e-Paper Clear\r\n ");
  //epd.Clear(EPD_5IN65F_WHITE);
  
//  Serial.print("draw image\r\n ");
  //epd.EPD_5IN65F_Display_part(gImage_5in65f, 204, 153, 192, 143);
//  epd.EPD_5IN65F_Show7Block();
//  delay(2000);

  Serial.print("display init\r\n ");
  display.init(115200);  // Initiate the display
  delay(2000);
  Serial.print("display cleared\r\n ");
  
  display.setRotation(0);  // Set orientation. Goes from 0, 1, 2 or 3
//  display.setCursor(0,0);
  
  //display.setTextWrap(false);  // By default, long lines of text are set to automatically “wrap” back to the leftmost column.
  display.setFullWindow();  // Set full window mode, meaning is going to update the entire screen

  display.fillScreen(GxEPD_WHITE);  // Clear previous graphics to start over to print new things.
//  Serial.print("blank out the screen\r\n ");
//  display.display();
//  delay(2000);
  
  // Print text - "Hello World!":
  display.setTextColor(GxEPD_BLACK);  // Set color for text
  display.setFont(&FreeMono12pt7b);  // Set font
  //display.setTextSize(12);
  display.setCursor(0, 12);  // Set the position to start printing text (x,y)
  display.println("Hello World!");  // Print some text
  Serial.print("Hello World\r\n ");

  // Draw triangle:
  display.drawTriangle(0,85,   20,60,   40,85,   GxEPD_BLACK);  // Draw triangle. X, Y coordinates for three corner points defining the triangle, followed by a color
  
  // Draw filled triangle:
  display.fillTriangle(110,85,   130,60,   150,85,   GxEPD_BLACK);  // Draw filled triangle. X, Y coordinates for three corner points defining the triangle, followed by a color
  Serial.print("triangle\r\n ");
  
  // Draw line:
  display.drawLine(74,20,   74,80,   GxEPD_BLACK);  // Draw line (x0,y0,x1,y1,color)
  Serial.print("line\r\n ");
  
  // Draw rounded rectangle and fill:
  display.fillRoundRect(48,60,   20,25,   5, GxEPD_BLACK);  // Draw filled rounded rectangle (x,y,width,height,color)
                                                 // It draws from the location to down-right
  Serial.print("rect\r\n ");
  
  // Draw circle:
  display.drawCircle(95,70,   15, GxEPD_BLACK);  //  Draw circle (x,y,radius,color). X and Y are the coordinates for the center point
  
  // Draw a filled circle:
  display.fillCircle(100,75,   7, GxEPD_BLACK);  // Draw filled circle (x,y,radius,color). X and Y are the coordinates for the center point
  Serial.print("circle\r\n ");
  
  // Draw rectangle:
  display.drawRect(8,25,   49,27, GxEPD_BLACK);  // Draw rectangle (x,y,width,height,color)
  Serial.print("rect\r\n ");

  delay(1000);
  display.display();
  Serial.print("display\r\n ");
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
}

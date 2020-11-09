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

#include <time.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduinoJson.h>

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
#include "Roboto_Regular78pt7b.h"

// seems Adafruit_GFX maybe only supports 3 colors but we have a 7 color display... we'll find out
GxEPD2_3C<GxEPD2_565c, GxEPD2_565c::HEIGHT> ePaperDisplay(GxEPD2_565c(/*CS=10*/ CS_PIN, /*DC=*/ DC_PIN, /*RST=*/ RST_PIN, /*BUSY=*/ BUSY_PIN)); // GDEW075Z08 800x480
//#endif
time_t lastSecond;

// returns 0 no updates, returns 1 only lcd updates, returns 2 both lcd and epaper updates
// normalDisplay is so we can run this in the main loop for an lcd but also in an epaper loop
short displayTime(short needUpdate, time_t currentSecond, const char *quote, const char *author, const int temp) {
  char buf[80];
  const time_t rawtime = currentSecond;
  const int rawMinute = (int)(rawtime/60);
  const int lastMinute = (int)(lastSecond/60);
  Serial.print("lastSecond: ");
  Serial.println((long)lastSecond);
  Serial.print("rawtime: ");
  Serial.println((long)rawtime);
  lastSecond = rawtime;
  if (needUpdate ||  rawMinute > lastMinute) {
    const bool epaper_update = (rawMinute > lastMinute);
    //snprintf(buffer, 1024,
    //         "rawtime: %ld, lastSecond: %ld, rm: %d lm: %d\n", rawtime, lastSecond, rawMinute, lastMinute);
    //Serial.print(buffer);
    struct tm  ts;
    ts = *localtime(&rawtime);
    strftime(buf, sizeof(buf), "%l:%M %p\n", &ts);
    if (epaper_update) {
      Serial.print("update epaper with buf: ");
      Serial.println(buf);
      //ePaperDisplay.getTextBounds(buf, 0, 32, &x1, &y1, &w, &h);
      //ePaperDisplay.fillRect(0, 32, w, h, GxEPD_WHITE);
      ePaperDisplay.fillScreen(GxEPD_WHITE);  // Clear previous graphics to start over to print new things.
      //ePaperDisplay.setPartialWindow(0, 0, ePaperDisplay.width(), ePaperDisplay.height());
      ePaperDisplay.setFont(&Roboto_Regular78pt7b);  // Set font
      ePaperDisplay.setCursor(80, 220);  // Set the position to start printing text (x,y)
      strftime(buf, sizeof(buf), "%l:%M", &ts);
      ePaperDisplay.println(buf);  // Print some text
    }
    strftime(buf, sizeof(buf), "%a %b, %d\n", &ts);
    if (epaper_update) {
      /*int16_t  x1, y1;
      uint16_t w, h;
      ePaperDisplay.setFont(&FreeMono18pt7b);  // Set font
      ePaperDisplay.getTextBounds(buf, 0, 32, &x1, &y1, &w, &h);
      ePaperDisplay.fillRect(0, 32, w, h, GxEPD_WHITE);
      */
      ePaperDisplay.setCursor(25, 50);  // Set the position to start printing text (x,y)
      ePaperDisplay.setFont(&FreeMono18pt7b);  // Set font
      ePaperDisplay.println(buf);  // print the date
      Serial.print("update epaper with buf: ");
      Serial.println(buf);
      //yield();
      // print a quote if we have one
      if (strlen(quote) > 0) {
        ePaperDisplay.setCursor(0, 290);  // Set the position to start printing text (x,y)
        ePaperDisplay.setFont(&FreeMono12pt7b);  // Set font
        ePaperDisplay.println(quote);  // print the quote
        ePaperDisplay.print("by: ");  // print the author
        ePaperDisplay.println(author);  // print the author
        Serial.print("update epaper with quote & author: ");
        Serial.println(quote);
        Serial.println(author);
      }
      // display temperature
      ePaperDisplay.setCursor(480, 50);
      ePaperDisplay.setFont(&FreeMono18pt7b);
      ePaperDisplay.print(temp);
      ePaperDisplay.println("F");
      return 2;
    }
    return 1;
  }
  lastSecond = rawtime;
  return 0;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  Serial1.begin(9600);

  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);
    
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
  ePaperDisplay.init(115200);  // Initiate the display
  delay(2000);
  Serial.print("display cleared\r\n ");
  
  ePaperDisplay.setRotation(0);  // Set orientation. Goes from 0, 1, 2 or 3
  ePaperDisplay.setCursor(0,0);
  
  ePaperDisplay.setTextWrap(true);  // By default, long lines of text are set to automatically “wrap” back to the leftmost column.
  ePaperDisplay.setFullWindow();  // Set full window mode, meaning is going to update the entire screen
  ePaperDisplay.setTextColor(GxEPD_BLACK);  // Set color for text

  ePaperDisplay.fillScreen(GxEPD_WHITE);  // Clear previous graphics to start over to print new things.
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial1.available()) {
    // Allocate the JSON document
    // This one must be bigger than for the sender because it must store the strings
    StaticJsonDocument<2048> doc;

    // Read the JSON document from the "link" serial port
    DeserializationError err = deserializeJson(doc, Serial1);

    if (err == DeserializationError::Ok) {
      digitalWrite(A0, HIGH);
      Serial.println("received doc");
      //rainbow(10);             // Flowing rainbow cycle along the whole strip

      /*
       sender is sending:
        doc["time"] = 1604759200;
        doc["temp"] = 52;
        doc["quote"] = "Leadership has a harder job to do than just choose sides. It must bring sides together.";
        doc["author"] = "cool beans";
  */
      // Print the values
      // (we must use as<T>() to resolve the ambiguity)
      long time = doc["time"].as<long>();
      int temp = doc["temp"].as<int>();
      if (time > 0) {
        Serial.print("time = ");
        Serial.println(time);
        Serial.print("temp = ");
        Serial.println(temp);

        const String quote = doc["quote"].as<String>();
        const String author = doc["author"].as<String>();

        Serial.print("quote = ");
        Serial.println(quote);

        Serial.print("author = ");
        Serial.println(author);
 
        if (displayTime(1, time, quote.c_str(), author.c_str(), temp) == 2) {
          ePaperDisplay.display();
        }
      }
      digitalWrite(A0, LOW);
    } else {
      digitalWrite(A0, LOW);
      // Print error to the "debug" serial port
      Serial.print("deserializeJson() returned ");
      Serial.println(err.c_str());

      // Flush all bytes in the "link" serial port buffer
      while (Serial1.available() > 0)
        Serial1.read();
    }
  }
}

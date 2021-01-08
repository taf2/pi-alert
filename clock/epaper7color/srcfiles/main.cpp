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
#include <StreamUtils.h>
#include <ArduinoJson.h>
#include <Adafruit_DotStar.h>

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

#define NUMPIXELS 1
#define DATAPIN 8
#define CLOCKPIN 6

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);

// seems Adafruit_GFX maybe only supports 3 colors but we have a 7 color display... we'll find out
GxEPD2_3C<GxEPD2_565c, GxEPD2_565c::HEIGHT> ePaperDisplay(GxEPD2_565c(/*CS=10*/ CS_PIN, /*DC=*/ DC_PIN, /*RST=*/ RST_PIN, /*BUSY=*/ BUSY_PIN)); // GDEW075Z08 800x480
//#endif
const unsigned short MaxTextWidth = 528; // max length that we want to allow a line to draw
const unsigned short MaxChars = 38; // we computed 38 characters was 528 length line and that seems good

// returns 0 no updates, returns 1 only lcd updates, returns 2 both lcd and epaper updates
// normalDisplay is so we can run this in the main loop for an lcd but also in an epaper loop
void displayTime(const char *quote,
                 const char *author,
                 const int temp,
                 const char *formattedTime,
                 const char *ampm,
                 const char *formattedDate) {

  ePaperDisplay.fillScreen(GxEPD_WHITE);
  ePaperDisplay.setFont(&Roboto_Regular78pt7b);
  ePaperDisplay.setCursor(60, 220);
  ePaperDisplay.print(formattedTime);
  ePaperDisplay.setFont(&FreeMonoBold24pt7b);
  ePaperDisplay.setCursor(410, 140);
  ePaperDisplay.println(ampm);

  ePaperDisplay.setCursor(25, 50);
  ePaperDisplay.setFont(&FreeMono18pt7b);
  ePaperDisplay.println(formattedDate);
  if (strlen(quote) > 0) {
    ePaperDisplay.setCursor(25, 270);  // Set the position to start printing text (x,y)
    ePaperDisplay.setFont(&FreeMono12pt7b);  // Set font
    {
      //int16_t x1, y1;
      //uint16_t w, h;
      //ePaperDisplay.getTextBounds("The very essence of leadership is that", 25, 290, &x1, &y1, &w, &h);
      /*Serial.println("bounding box for a max length line:");
      Serial.println(x1);
      Serial.println(y1);
      Serial.println(w);
      Serial.println(h);
      */
      char *outputQuote = (char*)quote;
      char outputBuffer[40];
      while (strlen(outputQuote) > MaxChars) {
        memcpy(outputBuffer, outputQuote, MaxChars);
        outputBuffer[MaxChars] = '\0';
        if (outputQuote[0] == ' ') {
          ePaperDisplay.setCursor(15, ePaperDisplay.getCursorY());  // Set the position to start printing text (x,y)
        } else {
          ePaperDisplay.setCursor(25, ePaperDisplay.getCursorY());  // Set the position to start printing text (x,y)
        }
        ePaperDisplay.println(outputBuffer);  // print out what ever is left over
        outputQuote += MaxChars; // move the pointer to max chars
      }
      if (strlen(outputQuote)) {
        if (outputQuote[0] == ' ') {
          ePaperDisplay.setCursor(15, ePaperDisplay.getCursorY());  // Set the position to start printing text (x,y)
        } else {
          ePaperDisplay.setCursor(25, ePaperDisplay.getCursorY());  // Set the position to start printing text (x,y)
        }
        ePaperDisplay.println(outputQuote);  // print out what ever is left over
      }
      /*do {
        ePaperDisplay.getTextBounds(quote, 25, 290, &x1, &y1, &w, &h);
        if (w > 528) {
            
        }
      } while (w > 528);
      */

      /*
       *
        x1 -> 27
        y1 -> 276
        w -> 528
        h -> 19
       */
      //ePaperDisplay.println(quote);  // print the quote
    }
    //ePaperDisplay.print("by: ");  // print the author
    //ePaperDisplay.println("");  // print the author
    ePaperDisplay.setCursor(50, ePaperDisplay.getCursorY()+19);  // Set the position to start printing text (x,y)
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
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
//  while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  strip.begin();
  Serial1.begin(9600);
/*  while (!Serial1) {
    strip.setPixelColor(0, 16, 0, 0);
    strip.show();
    delay(100);
    strip.setPixelColor(0, 0, 0, 16);
    strip.show();
  }*/

  strip.setPixelColor(0, 0, 0, 16);
  strip.show();

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
// this helps reset the display if we're seeing ghosting
//  ePaperDisplay.display();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial1.available()) {
    // Allocate the JSON document
    // This one must be bigger than for the sender because it must store the strings
    StaticJsonDocument<2048> doc;

    // Read the JSON document from the "link" serial port
    DeserializationError err = deserializeJson(doc, Serial1);

    strip.setPixelColor(0, 127, 255, 0);
    strip.show();

    if (err == DeserializationError::Ok) {
      strip.setPixelColor(0, 0, 255, 0);
      strip.show();
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
      bool clear = doc["clear"].as<bool>();
      if (clear) {
        strip.setPixelColor(0, 0, 0, 255);
        strip.show();
        ePaperDisplay.fillScreen(GxEPD_WHITE);  // Clear previous graphics to start over to print new things.
      // this helps reset the display if we're seeing ghosting
        ePaperDisplay.display();
        Serial1.write("0");
      } else {
        Serial.println("display to screen");
        long time = doc["time"].as<long>();
        int temp = doc["temp"].as<int>();
        if (time > 0) {
          Serial.print("time = ");
          Serial.println(time);
          Serial.print("temp = ");
          Serial.println(temp);

          const String quote = doc["quote"].as<String>();
          const String author = doc["author"].as<String>();
          const String ftime = doc["ftime"].as<String>();
          const String ampm = doc["ampm"].as<String>();
          const String fdate = doc["fdate"].as<String>();

          Serial.print("quote = ");
          Serial.println(quote);

          Serial.print("author = ");
          Serial.println(author);

          Serial.print("ftime = ");
          Serial.println(ftime);
   
          displayTime(quote.c_str(), author.c_str(), temp, ftime.c_str(), ampm.c_str(), fdate.c_str());
          ePaperDisplay.display();
          Serial.println("display success");
          Serial1.write("0");
        } else {
          Serial.println("received a 0 time doc???");
          Serial1.write("1");
        }
      }
      digitalWrite(A0, LOW);
    } else {
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
      digitalWrite(A0, LOW);
      // Print error to the "debug" serial port
      Serial.print("deserializeJson() returned ");
      Serial.println(err.c_str());

      // Flush all bytes in the "link" serial port buffer
      while (Serial1.available() > 0) {
        Serial1.read();
      }
    }
  } else {
    strip.setPixelColor(0, 0, 0, 8);
    strip.show();
  }
}

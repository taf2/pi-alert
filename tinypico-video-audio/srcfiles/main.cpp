#define OV5642_MINI_5MP_PLUS

#include <TinyPICO.h>

#include <SPI.h>
#include <mySD.h>

#include <ArduCAM.h>

#include <WiFi.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

#define   FRAMES_NUM    0x00

void blink(int pin, int seconds);
void printLine();
void requestURL(const char * host, uint8_t port);
void connectToWiFi(const char * ssid, const char * pwd);

const int CS = 5; // camera CS pin
// Argh we don't have enough pins we need MISO/MOSI
// these pins overlap with the camera
const int SD_CLK = 26; // SCK
const int SD_DO  = 14; // MO
const int SD_DI  = 15; // MI
const int SD_CS  = 32; // SS
//const int SD_CD  = 33; // optional card disconnected..
#define SDSPEED 27000000

const int MOTION_LED_PIN = 4; // output led to indicate we have detected motion
const int ON_LED_PIN = 27; // output led to indicate we're running
const int MOTION_PIN = 25; // input to tell us we have detected motion

const char *ssid = "<%= @config[:ssid] %>";
const char *pass = "<%= @config[:pass] %>";
const char *node = "<%= @config[:events][:name] %>";

uint8_t resolution = OV5642_640x480;//OV5642_2592x1944;

ArduCAM myCAM(OV5642, CS);

static const size_t bufferSize = 4096;
static uint8_t buffer[bufferSize] = {0xFF};
static char hexBuffer[1024];

// Initialise the TinyPICO library
TinyPICO tp = TinyPICO();

void SD_setup() {

  // https://github.com/espressif/arduino-esp32/issues/1219
  //pinMode(SD_CS, OUTPUT);
  while (!SD.begin(SD_CS, SD_DI, SD_DO, SD_CLK)) {
    blink(ON_LED_PIN, 5);
    delay(1000);
    ESP.restart();
  }
}

void SPI_setup() {
  uint8_t temp;
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  // set the CS as an output:
  // initialize SPI:
  do {
    Serial.println(F("ArduCAM Start!"));
    SPI.begin();
    SPI.setFrequency(4000000); //4MHz
    //Check if the ArduCAM SPI bus is OK
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55) {
      Serial.println(F("SPI1 interface Error!"));
      blink(ON_LED_PIN, 5);
    } else {
      break;
    }
  } while(1);
}

int camCapture(ArduCAM myCAM, int sequence) {
  WiFiClient client;
  myCAM.clear_fifo_flag();
  unsigned int i = 0;
  bool is_header = false;
  uint8_t temp = 0, temp_last = 0;

  if (!client.connect("<%= @config[:events][:host] %>", <%= @config[:events][:port] %>)) {
    Serial.println(F("Connection error to <%= @config[:events][:host] %>:<%= @config[:events][:port] %>"));
    blink(ON_LED_PIN, 10);
    return -1;
  }
  Serial.println("connected starting read and upload");

  uint32_t len  = myCAM.read_fifo_length();
  if (len >= MAX_FIFO_SIZE) { //8M
    Serial.println(F("Over size."));
    blink(ON_LED_PIN, 15);
    return -1;
  }
  if (len == 0 ) { //0 kb
    Serial.println(F("Size is 0."));
    blink(ON_LED_PIN, 20);
    return -1;
  }
  client.setNoDelay(true);
  client.println("POST /capture HTTP/1.1");
  client.println("Host: <%= @config[:events][:host] %>");
  client.println("Connection: close");
  client.println("Content-Type: image/jpeg");
  snprintf(hexBuffer, 1024, "X-Node: %s", node);
  client.println(hexBuffer);
  snprintf(hexBuffer, 1024, "X-Seq: %d", sequence);
  client.println(hexBuffer);
  client.println("Transfer-Encoding: chunked");

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  if (!client.connected()) {
    blink(ON_LED_PIN, 25);
    return -1;
  }

  String buftest; // for testing

  while (len--) {
    temp_last = temp;
    temp =  SPI.transfer(0x00);
    //Read JPEG data from FIFO
    if ( (temp == 0xD9) && (temp_last == 0xFF) ) { //If find the end ,break while,
      buffer[i++] = temp;  //save the last  0XD9
      //Write the remain bytes in the buffer
      if (!client.connected()) { break; }
      snprintf(hexBuffer, 1024, "\r\n%X\r\n", i);
      client.print(hexBuffer);

      //client.print(buftest.c_str());
      client.write(&buffer[0], i);
      is_header = false;
      i = 0;
      myCAM.CS_HIGH();
      break;
    }

    if (is_header == true) {
      //Write image data to buffer if not full
      if (i < bufferSize) {
        buffer[i++] = temp;
      } else {
        //Write bufferSize bytes image data to file
        if (!client.connected()) { break; }
        snprintf(hexBuffer, 1024, "\r\n%X\r\n", bufferSize);
        client.print(hexBuffer);

        client.write(&buffer[0], bufferSize);
        i = 0;
        buffer[i++] = temp;
      }
    } else if ((temp == 0xD8) & (temp_last == 0xFF)) {
      is_header = true;
      buffer[i++] = temp_last;
      buffer[i++] = temp;
    }
  }
  client.print("\r\n0\r\n\r\n");
  client.stop();
  return 0;
}

void postEvent() {
  WiFiClient client;
  HTTPClient http;
  String postData = "{\"hello\":\"world\"}";
  String url = "http://<%= @config[:events][:host] %>:<%= @config[:events][:port] %>/event";
  Serial.println("POST:");
  Serial.println(url.c_str());

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.POST(postData);
  http.writeToStream(&Serial);
  http.end();

}

// the issue with motion is when we trigger if nothing is yet in the viewable frame we'll miss it.
// so instead here we want to capture a short 10 second video to increase the probablity that whatever 
// tripped the motion sensor will be caught in a frame.  A possible issue is we won't be saving to sd card (at least not yet)
// and we might not have enough memory to store all the frames in memory... we'd like to solve this by streaming to our
// server but eventually we'll probably need/want an sd card for buffer in case we're not connected to wifi
int videoMotionCapture() {
// maybe use https://github.com/dmainmon/ArduCAM-mini-ESP8266-12E-Camera-Server/blob/master/ArduCam_ESP8266_FileCapture.ino
// e.g. capture a bunch of snapshots using local SPIFFS and then flush captured images uploading them to server in a batch
	return 0;
}

int motionCapture(int sequence) {
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
//  arducam_set_control(myCAM, V4L2_CID_FOCUS_ABSOLUTE, 190);

  myCAM.start_capture();

  Serial.println(F("CAM Capturing"));
  int total_time = 0;
  total_time = millis();
  while ( !myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)); 

  total_time = millis() - total_time;
  Serial.print(F("capture total_time used (in miliseconds):"));
  Serial.println(total_time, DEC);
  total_time = 0;
  Serial.println(F("CAM Capture Done."));
  total_time = millis();
  if (!camCapture(myCAM, sequence)) {
    total_time = millis() - total_time;
    Serial.print(F("send total_time used (in miliseconds):"));
    Serial.println(total_time, DEC);
    Serial.println(F("CAM send Done."));
    myCAM.clear_fifo_flag();
    return 0;
  } else {
    Serial.println(F("Error sending."));
    myCAM.clear_fifo_flag();
    return -1;
  }
}

void blink(int pin, int seconds) {
  for (int i = 0; i < seconds; ++i) {
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(CS, OUTPUT);
  pinMode(MOTION_LED_PIN, OUTPUT);
  pinMode(ON_LED_PIN, OUTPUT);
  pinMode(MOTION_PIN, INPUT);

  SD_setup();
  SPI_setup();


  blink(ON_LED_PIN, 5);
  blink(MOTION_LED_PIN, 5);

  digitalWrite(ON_LED_PIN, HIGH);
  digitalWrite(MOTION_LED_PIN, HIGH);

  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  myCAM.OV5642_set_JPEG_size(resolution);

  connectToWiFi(ssid, pass);

  tp.DotStar_SetPower( false );

  digitalWrite(ON_LED_PIN, HIGH);
  digitalWrite(MOTION_LED_PIN, LOW);
}

int pirState = LOW; // assume start no motion
int sequence = 0;

void loop() {

  long motion = digitalRead(MOTION_PIN);
  if (motion == HIGH) {
    Serial.println("motion detected");
    digitalWrite(MOTION_LED_PIN, HIGH);
    if (pirState == LOW) {
      pirState = HIGH; // just turned HIGH

      sequence++;

      if (!motionCapture(sequence)) {
        yield();
        Serial.println("capturing 10 more frames from motion trigger");
        for (int i = 0; i < 10; ++i) {
          motionCapture(sequence);
          yield();
        }
        //digitalWrite(MOTION_LED_PIN, LOW);
      } else {
        Serial.println("capture error flash led to signal error, 29 seconds");
          
        blink(ON_LED_PIN, 2);
        blink(MOTION_LED_PIN, 10); // 10 seconds blinking
        blink(ON_LED_PIN, 2);

        Serial.println("error state low idle 5 seconds");
        digitalWrite(MOTION_LED_PIN, LOW);
        delay(5000);

        blink(MOTION_LED_PIN, 10); // 10 seconds blinking
      }
    }
  } else {
    digitalWrite(MOTION_LED_PIN, LOW);
    if (pirState == HIGH) {
      pirState = LOW;
    }

  }
  delay(100);
}

void connectToWiFi(const char * ssid, const char * pwd) {
  int ledState = 0;

  printLine();
  Serial.println("Connecting to WiFi network: " + String(ssid));

  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED)
  {
    // Blink LED while we're connecting:
    digitalWrite(MOTION_LED_PIN, ledState);
    ledState = (ledState + 1) % 2; // Flip ledState
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void requestURL(const char * host, uint8_t port) {
  printLine();
  Serial.println("Connecting to domain: " + String(host));

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }
  Serial.println("Connected!");
  printLine();

  // This will send the request to the server
  client.print((String)"GET / HTTP/1.1\r\n" +
               "Host: " + String(host) + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
  client.stop();
}

void printLine() {
  Serial.println();
  for (int i=0; i<30; i++)
    Serial.print("-");
  Serial.println();
}
